/*
 *  Copyright (C) 2021 Express VPN International Ltd.
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "msg_handlers.h"

#include "conn.h"
#include "core.h"

he_return_code_t he_handle_msg_noop(he_conn_t *conn, uint8_t *packet, int length) {
  if(conn == NULL || packet == NULL) {
    return HE_ERR_NULL_POINTER;
  }

  // Don't need to do anything for this message type
  return HE_SUCCESS;
}

he_return_code_t he_handle_msg_ping(he_conn_t *conn, uint8_t *packet, int length) {
  if(conn == NULL || packet == NULL) {
    return HE_ERR_NULL_POINTER;
  }

  // We only want to do stuff with pings when the connection is ONLINE

  if(conn->state != HE_STATE_ONLINE) {
    return HE_ERR_INVALID_CLIENT_STATE;
  }

  // Create the pong here
  he_msg_pong_t response = {0};
  response.msg_header.msgid = HE_MSGID_PONG;

  // Send pong
  he_internal_send_message(conn, (uint8_t *)&response, sizeof(he_msg_pong_t));

  return HE_SUCCESS;
}

he_return_code_t he_handle_msg_pong(he_conn_t *conn, uint8_t *packet, int length) {
  if(conn == NULL || packet == NULL) {
    return HE_ERR_NULL_POINTER;
  }

  // Tell the host application that we received a PONG
  he_internal_generate_event(conn, HE_EVENT_PONG);
  return HE_SUCCESS;
}

he_return_code_t he_handle_msg_config_ipv4(he_conn_t *conn, uint8_t *packet, int length) {
  if(conn == NULL || packet == NULL) {
    return HE_ERR_NULL_POINTER;
  }

  // Check that we're in the right state to receive a config
  if(conn->is_server || conn->state != HE_STATE_AUTHENTICATING) {
    return HE_ERR_INVALID_CLIENT_STATE;
  }

  // Check the packet is going to be big enough
  if(length < sizeof(he_msg_config_ipv4_t)) {
    return HE_ERR_PACKET_TOO_SMALL;
  }

  // Cast the packet
  he_msg_config_ipv4_t *pkt = (he_msg_config_ipv4_t *)packet;

  // Set up some space for the config callback
  he_network_config_ipv4_t config = {0};

  // Safely copy the values out and ensure null termination
  strncpy(config.local_ip, pkt->local_ip, HE_MAX_IPV4_STRING_LENGTH);
  config.local_ip[HE_MAX_IPV4_STRING_LENGTH - 1] = '\0';

  strncpy(config.peer_ip, pkt->peer_ip, HE_MAX_IPV4_STRING_LENGTH);
  config.peer_ip[HE_MAX_IPV4_STRING_LENGTH - 1] = '\0';

  strncpy(config.dns_ip, pkt->dns_ip, HE_MAX_IPV4_STRING_LENGTH);
  config.dns_ip[HE_MAX_IPV4_STRING_LENGTH - 1] = '\0';

  int parsed_mtu_value;
  // Before calling sscanf null-terminate the string
  pkt->mtu[HE_MAX_IPV4_STRING_LENGTH - 1] = '\0';
  // Note that res is the "number of variables populated", so we need to check for 1
  int res = sscanf(pkt->mtu, "%u", &parsed_mtu_value);

  if(res != 1 || parsed_mtu_value <= 0 || parsed_mtu_value > HE_MAX_MTU) {
    config.mtu = HE_MAX_MTU;
  } else {
    config.mtu = parsed_mtu_value;
  }

  // Copy out the session ID
  conn->session_id = pkt->session;

  // Change state to configuring...
  he_internal_change_conn_state(conn, HE_STATE_CONFIGURING);

  // Call configure callback if set
  if(conn->network_config_ipv4_cb) {
    // Check the callback returned successfully
    if(conn->network_config_ipv4_cb(conn, &config, conn->data) != HE_SUCCESS) {
      // Return error without changing state. It's client-side app's responsibility to
      // call `he_client_disconnect` when seeing HE_ERR_CALLBACK_FAILED error from
      // he_client_outside_packet_received function.
      return HE_ERR_CALLBACK_FAILED;
    }
  }

  // All went well so...
  he_internal_change_conn_state(conn, HE_STATE_ONLINE);

  return HE_SUCCESS;
}

he_return_code_t he_handle_msg_auth(he_conn_t *conn, uint8_t *packet, int length) {
  if(conn == NULL || packet == NULL) {
    return HE_ERR_NULL_POINTER;
  }

  // Check we're in the right state:
  // 1. We must be a server
  // 2. We must be in either LINK_UP or ONLINE
  if(!conn->is_server && (conn->state != HE_STATE_LINK_UP && conn->state != HE_STATE_ONLINE)) {
    return HE_ERR_INVALID_CLIENT_STATE;
  }

  // Check that we actually have an auth handler setup
  if(conn->auth_cb == NULL || conn->populate_network_config_ipv4_cb == NULL) {
    return HE_ERR_INVALID_CLIENT_STATE;
  }

  // Check the packet is big enough
  if(length < sizeof(he_msg_auth_t)) {
    return HE_ERR_PACKET_TOO_SMALL;
  }

  // Cast header
  he_msg_auth_t *msg = (he_msg_auth_t *)packet;

  bool auth_state = conn->auth_cb(conn, msg->username, msg->password, conn->data);

  if(!auth_state) {
    he_internal_change_conn_state(conn, HE_STATE_DISCONNECTING);
    return HE_ERR_ACCESS_DENIED;
  }

  // At this point the user is authenticated
  // We no longer need msg->password, let's zero it out
  memset(msg->password, 0, HE_CONFIG_TEXT_FIELD_LENGTH);

  // Copy username into the connection. Force NULL terminator after since strncpy does not insert 0
  // if username is HE_CONFIG_TEXT_FIELD_LENGTH exactly
  strncpy(conn->username, msg->username, HE_CONFIG_TEXT_FIELD_LENGTH);
  conn->username[HE_CONFIG_TEXT_FIELD_LENGTH] = 0;

  // Create config to send to the client

  // Allocate some space for the config message
  uint8_t msg_buffer[sizeof(he_msg_config_ipv4_t)] = {0};

  he_msg_config_ipv4_t *response = (he_msg_config_ipv4_t *)&msg_buffer;
  response->msg_header.msgid = HE_MSGID_CONFIG_IPV4;

  // Copy the session ID into the response
  response->session = conn->session_id;

  // Set up some space for the config callback
  he_network_config_ipv4_t config = {0};

  // Copy the homogonized network configuration into the auth response
  int res = conn->populate_network_config_ipv4_cb(conn, &config, conn->data);
  if(res != HE_SUCCESS) {
    return res;
  }

  // Safely copy the values out and ensure null termination
  strncpy(response->local_ip, config.local_ip, HE_MAX_IPV4_STRING_LENGTH);
  response->local_ip[HE_MAX_IPV4_STRING_LENGTH - 1] = '\0';

  strncpy(response->peer_ip, config.peer_ip, HE_MAX_IPV4_STRING_LENGTH);
  response->peer_ip[HE_MAX_IPV4_STRING_LENGTH - 1] = '\0';

  strncpy(response->dns_ip, config.dns_ip, HE_MAX_IPV4_STRING_LENGTH);
  response->dns_ip[HE_MAX_IPV4_STRING_LENGTH - 1] = '\0';

  strncpy(response->mtu, HE_MAX_MTU_STR, sizeof(response->mtu));

  // Send config
  he_internal_send_message(conn, (uint8_t *)response, sizeof(he_msg_config_ipv4_t));

  // All went well so...
  he_internal_change_conn_state(conn, HE_STATE_ONLINE);

  return HE_SUCCESS;
}

he_return_code_t he_handle_msg_data(he_conn_t *conn, uint8_t *packet, int length) {
  if(conn == NULL || packet == NULL) {
    return HE_ERR_NULL_POINTER;
  }

  // Check we're in the ONLINE state
  if(conn->state != HE_STATE_ONLINE) {
    return HE_ERR_INVALID_CLIENT_STATE;
  }

  // Quick header check
  if(length < sizeof(he_msg_data_t)) {
    return HE_ERR_PACKET_TOO_SMALL;
  }

  // Apply header
  he_msg_data_t *pkt = (he_msg_data_t *)packet;

  // We use this a lot so convert it just the once
  // Prior to May 2021, a bug here passed the "length" in host-order instead of network order
  // We have fixed this as of protocol version 1.1 but still support the bug for older clients
  uint16_t pkt_length;
  if(conn->protocol_version.major_version == 1 && conn->protocol_version.minor_version == 0) {
    pkt_length = pkt->length;
  } else {
    pkt_length = ntohs(pkt->length);
  }

  // Check the packet length is sufficient
  if(pkt_length > length) {
    return HE_ERR_PACKET_TOO_SMALL;
  }

  // Validate packet
  if(!he_internal_is_ipv4_packet_valid(packet + sizeof(he_msg_data_t), pkt_length)) {
    // Invalid packet
    return HE_ERR_BAD_PACKET;
  }

  // Packet seems to be fine, hand it over
  if(conn->inside_write_cb) {
    conn->inside_write_cb(conn, packet + sizeof(he_msg_data_t), pkt_length, conn->data);
  }

  return HE_SUCCESS;
}

he_return_code_t he_handle_msg_auth_response_with_config(he_conn_t *conn, uint8_t *packet,
                                                         int length) {
  if(conn == NULL || packet == NULL) {
    return HE_ERR_NULL_POINTER;
  }

  return HE_SUCCESS;
}

he_return_code_t he_handle_msg_auth_response(he_conn_t *conn, uint8_t *packet, int length) {
  if(conn == NULL || packet == NULL) {
    return HE_ERR_NULL_POINTER;
  }

  // Right now this means the login failed, so just shutdown
  return HE_ERR_ACCESS_DENIED;
}

// Temporary home for this
bool he_internal_is_ipv4_packet_valid(uint8_t *packet, int length) {
  if(packet == NULL) {
    return false;
  }
  // for now just check that the packet is IPv4
  int proto = packet[0] >> 4;

  if(proto != 4) {
    return false;
  }

  // Assume it's good enough
  return true;
}
