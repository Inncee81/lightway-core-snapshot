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

#include <he.h>
#include "unity.h"
#include "test_defs.h"

// Unit under test
#include "conn.h"

// Direct Includes for Utility Functions
#include "config.h"
#include "memory.h"
#include "ssl_ctx.h"

// Internal Mocks
#include "mock_fake_dispatch.h"
#include "mock_wolf.h"

// External Mocks
#include "mock_ssl.h"
#include "mock_wolfio.h"
// TODO Research whether it's possible to directly use a Wolf header instead of our fake one
#include "mock_fake_rng.h"

he_ssl_ctx_t ssl_ctx;
he_conn_t conn;

WOLFSSL wolf_ssl;

void setUp(void) {
  conn.wolf_ssl = &wolf_ssl;
}

void tearDown(void) {
  memset(&ssl_ctx, 0, sizeof(he_ssl_ctx_t));

  memset(&conn, 0, sizeof(he_conn_t));

  memset(&wolf_ssl, 0, sizeof(WOLFSSL));
  call_counter = 0;
}

void test_valid_to_connect_not_null(void) {
  he_conn_t *test = NULL;
  int res1 = he_conn_is_valid_client(&ssl_ctx, test);
  TEST_ASSERT_EQUAL(HE_ERR_NULL_POINTER, res1);
}

void test_valid_to_connect_no_username(void) {
  he_conn_t *test = he_conn_create();
  int res1 = he_conn_is_valid_client(&ssl_ctx, test);
  TEST_ASSERT_EQUAL(HE_ERR_CONF_USERNAME_NOT_SET, res1);

  free(test);
}

void test_valid_to_connect_no_password(void) {
  he_conn_t *test = he_conn_create();
  int res1 = he_conn_set_username(test, "myuser");
  TEST_ASSERT_EQUAL(HE_SUCCESS, res1);

  int res2 = he_conn_is_valid_client(&ssl_ctx, test);
  TEST_ASSERT_EQUAL(HE_ERR_CONF_PASSWORD_NOT_SET, res2);

  free(test);
}

void test_valid_to_connect_no_mtu(void) {
  he_conn_t *test = he_conn_create();
  int res1 = he_conn_set_username(test, "myuser");
  TEST_ASSERT_EQUAL(HE_SUCCESS, res1);

  int res2 = he_conn_set_password(test, "mypass");
  TEST_ASSERT_EQUAL(HE_SUCCESS, res2);

  int res3 = he_conn_is_valid_client(&ssl_ctx, test);
  TEST_ASSERT_EQUAL(HE_ERR_CONF_MTU_NOT_SET, res3);

  free(test);
}

void test_valid_to_connect_incorrect_protocol(void) {
  he_conn_t *test = he_conn_create();
  int res1 = he_conn_set_username(test, "myuser");
  TEST_ASSERT_EQUAL(HE_SUCCESS, res1);

  int res2 = he_conn_set_password(test, "mypass");
  TEST_ASSERT_EQUAL(HE_SUCCESS, res2);

  int res3 = he_conn_set_protocol_version(test, 0xFF, 0xFF);
  TEST_ASSERT_EQUAL(HE_SUCCESS, res3);

  int res4 = he_conn_set_outside_mtu(test, HE_MAX_WIRE_MTU);
  TEST_ASSERT_EQUAL(HE_SUCCESS, res4);

  int res5 = he_conn_is_valid_client(&ssl_ctx, test);
  TEST_ASSERT_EQUAL(HE_ERR_INCORRECT_PROTOCOL_VERSION, res5);

  free(test);
}

void test_conn_create_destroy(void) {
  he_conn_t *test_conn = NULL;

  test_conn = he_conn_create();

  TEST_ASSERT_NOT_NULL(test_conn);

  test_conn->wolf_ssl = &wolf_ssl;

  wolfSSL_free_Expect(&wolf_ssl);
  he_return_code_t ret = he_conn_destroy(test_conn);

  TEST_ASSERT_EQUAL_INT(HE_SUCCESS, ret);
}

void test_set_username(void) {
  int res = he_conn_set_username(&conn, good_username);
  TEST_ASSERT_EQUAL_STRING(good_username, conn.username);
  TEST_ASSERT_EQUAL(HE_SUCCESS, res);
}

void test_set_username_with_long_string(void) {
  int res = he_conn_set_username(&conn, bad_string_too_long);
  TEST_ASSERT_EQUAL_STRING("", conn.username);
  TEST_ASSERT_EQUAL(HE_ERR_STRING_TOO_LONG, res);
}

void test_set_username_with_empty_string(void) {
  int res = he_conn_set_username(&conn, "");
  TEST_ASSERT_EQUAL_STRING("", conn.username);
  TEST_ASSERT_EQUAL(HE_ERR_EMPTY_STRING, res);
}

void test_get_username(void) {
  he_conn_set_username(&conn, good_username);
  const char *test_username = he_conn_get_username(&conn);
  TEST_ASSERT_NOT_NULL(test_username);
  TEST_ASSERT_EQUAL_STRING(good_username, test_username);
}

void test_is_username_set(void) {
  bool res1 = he_conn_is_username_set(&conn);
  int res2 = he_conn_set_username(&conn, good_username);
  bool res3 = he_conn_is_username_set(&conn);

  TEST_ASSERT_EQUAL(false, res1);
  TEST_ASSERT_EQUAL(HE_SUCCESS, res2);
  TEST_ASSERT_EQUAL(true, res3);
}

// Handling Password

void test_set_password(void) {
  int res = he_conn_set_password(&conn, good_password);
  TEST_ASSERT_EQUAL_STRING(good_password, conn.password);
  TEST_ASSERT_EQUAL(HE_SUCCESS, res);
}

void test_set_password_with_long_string(void) {
  int res = he_conn_set_password(&conn, bad_string_too_long);
  TEST_ASSERT_EQUAL_STRING("", conn.password);
  TEST_ASSERT_EQUAL(HE_ERR_STRING_TOO_LONG, res);
}

void test_set_password_with_empty_string(void) {
  int res = he_conn_set_password(&conn, "");
  TEST_ASSERT_EQUAL_STRING("", conn.password);
  TEST_ASSERT_EQUAL(HE_ERR_EMPTY_STRING, res);
}

void test_is_password_set(void) {
  bool res1 = he_conn_is_password_set(&conn);
  int res2 = he_conn_set_password(&conn, good_password);
  bool res3 = he_conn_is_password_set(&conn);

  TEST_ASSERT_EQUAL(false, res1);
  TEST_ASSERT_EQUAL(HE_SUCCESS, res2);
  TEST_ASSERT_EQUAL(true, res3);
}

void test_set_mtu(void) {
  int res1 = he_conn_set_outside_mtu(&conn, 10);
  TEST_ASSERT_EQUAL(10, conn.outside_mtu);
  TEST_ASSERT_EQUAL(HE_SUCCESS, res1);
}

void test_get_mtu(void) {
  int res1 = he_conn_set_outside_mtu(&conn, 10);
  TEST_ASSERT_EQUAL(10, conn.outside_mtu);
  TEST_ASSERT_EQUAL(HE_SUCCESS, res1);

  int res2 = he_conn_get_outside_mtu(&conn);
  TEST_ASSERT_EQUAL(10, res2);
}

void test_is_mtu_set(void) {
  bool res1 = he_conn_is_outside_mtu_set(&conn);
  int res2 = he_conn_set_outside_mtu(&conn, 10);
  bool res3 = he_conn_is_outside_mtu_set(&conn);
  TEST_ASSERT_EQUAL(false, res1);
  TEST_ASSERT_EQUAL(10, conn.outside_mtu);
  TEST_ASSERT_EQUAL(HE_SUCCESS, res2);
  TEST_ASSERT_EQUAL(true, res3);
}

void test_set_context(void) {
  char test = 'x';

  he_conn_set_context(&conn, &test);
  TEST_ASSERT_EQUAL(&test, conn.data);
}

void test_get_context(void) {
  void *context = he_conn_get_context(&conn);
  TEST_ASSERT_NULL(context);
  int res1 = he_conn_set_context(&conn, fake_cert);
  TEST_ASSERT_EQUAL(HE_SUCCESS, res1);
  context = he_conn_get_context(&conn);
  TEST_ASSERT_EQUAL(fake_cert, context);
}

void test_dont_call_state_change_cb_for_same_state(void) {
  // Check the counter is at zero
  TEST_ASSERT_EQUAL(0, call_counter);

  // Set our test callback - just increments the counter
  conn.state_change_cb = state_cb;

  // Cause a state change
  he_internal_change_conn_state(&conn, HE_STATE_CONNECTING);
  // Check it incremented by 1
  TEST_ASSERT_EQUAL(1, call_counter);

  // Do it again
  he_internal_change_conn_state(&conn, HE_STATE_CONNECTING);
  // Check it incremented by 1
  TEST_ASSERT_EQUAL(1, call_counter);
}

void test_call_state_change_cb_for_new_states(void) {
  // Check the counter is at zero
  TEST_ASSERT_EQUAL(0, call_counter);

  // Set our test callback - just increments the counter
  conn.state_change_cb = state_cb;

  // Cause a state change
  he_internal_change_conn_state(&conn, HE_STATE_CONNECTING);
  // Check it incremented by 1
  TEST_ASSERT_EQUAL(1, call_counter);

  // Do it again
  he_internal_change_conn_state(&conn, HE_STATE_ONLINE);
  // Check it incremented by 1
  TEST_ASSERT_EQUAL(2, call_counter);
}

void test_get_nudge_time(void) {
  // Set this manually as it will be set by Wolf after a read callback
  conn.wolf_timeout = 5;
  int res1 = he_conn_get_nudge_time(&conn);
  TEST_ASSERT_EQUAL(5, res1);
}

void test_get_nudge_time_while_connected(void) {
  // Set this manually as it will be set by Wolf after a read callback
  conn.wolf_timeout = 5;
  conn.state = HE_STATE_ONLINE;

  int res1 = he_conn_get_nudge_time(&conn);
  TEST_ASSERT_EQUAL(0, res1);
}

void test_get_nudge_time_while_connected_renegotiating(void) {
  // Set this manually as it will be set by Wolf after a read callback
  conn.wolf_timeout = 5;
  conn.state = HE_STATE_ONLINE;
  conn.renegotiation_in_progress = true;

  int res1 = he_conn_get_nudge_time(&conn);
  TEST_ASSERT_EQUAL(5, res1);
}

void test_he_internal_update_timeout(void) {
  TEST_ASSERT_EQUAL(0, call_counter);

  wolfSSL_dtls_get_current_timeout_ExpectAndReturn(conn.wolf_ssl, 10);
  he_internal_update_timeout(&conn);
  TEST_ASSERT_EQUAL(0, call_counter);
}

void test_get_nudge_time_while_connected_renegotiating_bad_state(void) {
  // Set this manually as it will be set by Wolf after a read callback
  conn.wolf_timeout = 5;
  conn.state = HE_STATE_NONE;
  conn.renegotiation_in_progress = false;

  int res1 = he_conn_get_nudge_time(&conn);
  TEST_ASSERT_EQUAL(5, res1);
}

void test_he_internal_update_timeout_with_cb(void) {
  conn.nudge_time_cb = nudge_time_cb;

  wolfSSL_dtls_get_current_timeout_ExpectAndReturn(conn.wolf_ssl, 10);
  TEST_ASSERT_EQUAL(0, call_counter);
  he_internal_update_timeout(&conn);
  TEST_ASSERT_EQUAL(1, call_counter);
}

void test_he_nudge_connection_timed_out(void) {
  wolfSSL_dtls_got_timeout_ExpectAndReturn(conn.wolf_ssl, SSL_FATAL_ERROR);
  wolfSSL_get_error_ExpectAndReturn(conn.wolf_ssl, SSL_FATAL_ERROR, SSL_FATAL_ERROR);
  int res1 = he_conn_nudge(&conn);
  TEST_ASSERT_EQUAL(HE_CONNECTION_TIMED_OUT, res1);
}

void test_he_nudge(void) {
  wolfSSL_dtls_got_timeout_ExpectAndReturn(conn.wolf_ssl, SSL_SUCCESS);
  wolfSSL_dtls_get_current_timeout_ExpectAndReturn(conn.wolf_ssl, 10);
  int res1 = he_conn_nudge(&conn);
  TEST_ASSERT_EQUAL(HE_SUCCESS, res1);
}

void test_he_nudge_need_read(void) {
  wolfSSL_dtls_got_timeout_ExpectAndReturn(conn.wolf_ssl, SSL_FATAL_ERROR);
  wolfSSL_get_error_ExpectAndReturn(conn.wolf_ssl, SSL_FATAL_ERROR, SSL_ERROR_WANT_READ);
  wolfSSL_dtls_get_current_timeout_ExpectAndReturn(conn.wolf_ssl, 10);
  int res1 = he_conn_nudge(&conn);
  TEST_ASSERT_EQUAL(HE_SUCCESS, res1);
}

void test_he_nudge_need_write(void) {
  wolfSSL_dtls_got_timeout_ExpectAndReturn(conn.wolf_ssl, SSL_FATAL_ERROR);
  wolfSSL_get_error_ExpectAndReturn(conn.wolf_ssl, SSL_FATAL_ERROR, SSL_ERROR_WANT_WRITE);
  wolfSSL_dtls_get_current_timeout_ExpectAndReturn(conn.wolf_ssl, 10);
  int res1 = he_conn_nudge(&conn);
  TEST_ASSERT_EQUAL(HE_SUCCESS, res1);
}

void test_he_nudge_with_cb(void) {
  conn.nudge_time_cb = nudge_time_cb;
  wolfSSL_dtls_got_timeout_ExpectAndReturn(conn.wolf_ssl, SSL_SUCCESS);
  wolfSSL_dtls_get_current_timeout_ExpectAndReturn(conn.wolf_ssl, 10);
  int res = he_conn_nudge(&conn);
  TEST_ASSERT_EQUAL(HE_SUCCESS, res);
  TEST_ASSERT_EQUAL(1, call_counter);
}

void test_he_get_state(void) {
  // Change state
  he_internal_change_conn_state(&conn, HE_STATE_ONLINE);
  he_client_state_t state = he_conn_get_state(&conn);
  TEST_ASSERT_EQUAL(HE_STATE_ONLINE, state);
}

void test_he_disconnect(void) {
  conn.state = HE_STATE_ONLINE;
  wolfSSL_write_IgnoreAndReturn(SSL_SUCCESS);
  wolfSSL_shutdown_ExpectAndReturn(conn.wolf_ssl, SSL_SUCCESS);
  conn.state_change_cb = state_change_cb;
  TEST_ASSERT_EQUAL(0, call_counter);
  int res1 = he_conn_disconnect(&conn);
  TEST_ASSERT_EQUAL(2, call_counter);
  TEST_ASSERT_EQUAL(HE_SUCCESS, res1);
}

void test_send_keepalive_error_when_not_connected(void) {
  int res = he_conn_send_keepalive(&conn);
  TEST_ASSERT_EQUAL(HE_ERR_INVALID_CLIENT_STATE, res);
}

void test_send_keepalive_connected(void) {
  conn.state = HE_STATE_ONLINE;
  wolfSSL_write_IgnoreAndReturn(SSL_SUCCESS);
  int res = he_conn_send_keepalive(&conn);
  TEST_ASSERT_EQUAL(HE_SUCCESS, res);
}

void test_disconnect_reject_if_not_online(void) {
  wolfSSL_shutdown_IgnoreAndReturn(SSL_FATAL_ERROR);
  he_return_code_t res = he_conn_disconnect(&conn);
  TEST_ASSERT_EQUAL(HE_ERR_INVALID_CLIENT_STATE, res);
}

void test_no_segfault_on_disconnect_before_initialisation(void) {
  conn.wolf_ssl = NULL;
  wolfSSL_shutdown_IgnoreAndReturn(SSL_FATAL_ERROR);
  he_return_code_t res = he_conn_disconnect(&conn);
  TEST_ASSERT_EQUAL(HE_ERR_NEVER_CONNECTED, res);
}

void test_he_nudge_doesnt_trigger_callback_when_online(void) {
  // Should get through the first time, but not the second
  conn.nudge_time_cb = nudge_time_cb;
  wolfSSL_dtls_got_timeout_ExpectAndReturn(conn.wolf_ssl, SSL_SUCCESS);
  wolfSSL_dtls_get_current_timeout_ExpectAndReturn(conn.wolf_ssl, 10);
  he_return_code_t res = he_conn_nudge(&conn);
  TEST_ASSERT_EQUAL(1, call_counter);
  // Now we're online - callback shouldn't get called
  conn.state = HE_STATE_ONLINE;
  wolfSSL_dtls_got_timeout_ExpectAndReturn(conn.wolf_ssl, SSL_SUCCESS);
  res = he_conn_nudge(&conn);
  TEST_ASSERT_EQUAL(1, call_counter);
}

void test_he_send_auth_should_not_reject_in_authenticating_state(void) {
  conn.state = HE_STATE_AUTHENTICATING;
  wolfSSL_write_IgnoreAndReturn(100);
  he_return_code_t res = he_internal_send_auth(&conn);
  TEST_ASSERT_EQUAL(HE_SUCCESS, res);
}

void test_he_nudge_sends_auth_in_authenticating_state(void) {
  conn.state = HE_STATE_AUTHENTICATING;
  dispatch_ExpectAndReturn("he_internal_send_auth", HE_SUCCESS);
  wolfSSL_dtls_get_current_timeout_ExpectAndReturn(conn.wolf_ssl, 10);

  he_conn_nudge(&conn);
}

void test_calculate_data_padding_check_default_is_none(void) {
  size_t res = he_internal_calculate_data_packet_length(&conn, 10);
  TEST_ASSERT_EQUAL(10, res);
}

void test_calculate_data_padding_full_small(void) {
  conn.padding_type = HE_PADDING_FULL;
  size_t res = he_internal_calculate_data_packet_length(&conn, 10);
  TEST_ASSERT_EQUAL(HE_MAX_MTU, res);
}

void test_calculate_data_padding_full_medium(void) {
  conn.padding_type = HE_PADDING_FULL;
  size_t res = he_internal_calculate_data_packet_length(&conn, 460);
  TEST_ASSERT_EQUAL(HE_MAX_MTU, res);
}

void test_calculate_data_padding_full_large(void) {
  conn.padding_type = HE_PADDING_FULL;
  size_t res = he_internal_calculate_data_packet_length(&conn, 910);
  TEST_ASSERT_EQUAL(HE_MAX_MTU, res);
}

void test_calculate_data_padding_none_small(void) {
  conn.padding_type = HE_PADDING_NONE;
  size_t res = he_internal_calculate_data_packet_length(&conn, 10);
  TEST_ASSERT_EQUAL(10, res);
}

void test_calculate_data_padding_none_medium(void) {
  conn.padding_type = HE_PADDING_NONE;
  size_t res = he_internal_calculate_data_packet_length(&conn, 460);
  TEST_ASSERT_EQUAL(460, res);
}

void test_calculate_data_padding_none_large(void) {
  conn.padding_type = HE_PADDING_NONE;
  size_t res = he_internal_calculate_data_packet_length(&conn, 910);
  TEST_ASSERT_EQUAL(910, res);
}

void test_calculate_data_padding_small(void) {
  conn.padding_type = HE_PADDING_450;
  size_t res = he_internal_calculate_data_packet_length(&conn, 10);
  TEST_ASSERT_EQUAL(450, res);
}

void test_calculate_data_padding_medium(void) {
  conn.padding_type = HE_PADDING_450;
  size_t res = he_internal_calculate_data_packet_length(&conn, 460);
  TEST_ASSERT_EQUAL(900, res);
}

void test_calculate_data_padding_large(void) {
  conn.padding_type = HE_PADDING_450;
  size_t res = he_internal_calculate_data_packet_length(&conn, 910);
  TEST_ASSERT_EQUAL(HE_MAX_MTU, res);
}

void test_calculate_data_padding_small_edge(void) {
  conn.padding_type = HE_PADDING_450;
  size_t res = he_internal_calculate_data_packet_length(&conn, 450);
  TEST_ASSERT_EQUAL(450, res);
}

void test_calculate_data_padding_medium_edge(void) {
  conn.padding_type = HE_PADDING_450;
  size_t res = he_internal_calculate_data_packet_length(&conn, 900);
  TEST_ASSERT_EQUAL(900, res);
}

void test_calculate_data_padding_large_edge(void) {
  conn.padding_type = HE_PADDING_450;
  size_t res = he_internal_calculate_data_packet_length(&conn, HE_MAX_MTU);
  TEST_ASSERT_EQUAL(HE_MAX_MTU, res);
}

void test_internal_shutdown(void) {
  conn.state = HE_STATE_ONLINE;
  conn.outside_write_cb = write_cb;
  conn.inside_write_cb = write_cb;
  wolfSSL_write_IgnoreAndReturn(SSL_SUCCESS);
  wolfSSL_shutdown_ExpectAndReturn(conn.wolf_ssl, SSL_SUCCESS);
  he_internal_disconnect_and_shutdown(&conn);

  TEST_ASSERT_EQUAL(NULL, conn.outside_write_cb);
  TEST_ASSERT_EQUAL(NULL, conn.inside_write_cb);
}

void test_he_internal_send_message(void) {
  wolfSSL_write_ExpectAndReturn(conn.wolf_ssl, fake_ipv4_packet, sizeof(fake_ipv4_packet),
                                sizeof(fake_ipv4_packet));

  int res = he_internal_send_message(&conn, fake_ipv4_packet, sizeof(fake_ipv4_packet));
}

void test_he_internal_update_timeout_with_cb_multiple_calls(void) {
  conn.nudge_time_cb = nudge_time_cb;

  wolfSSL_dtls_get_current_timeout_ExpectAndReturn(conn.wolf_ssl, 10);
  wolfSSL_dtls_get_current_timeout_ExpectAndReturn(conn.wolf_ssl, 10);
  wolfSSL_dtls_got_timeout_ExpectAndReturn(conn.wolf_ssl, 1);
  wolfSSL_dtls_get_current_timeout_ExpectAndReturn(conn.wolf_ssl, 10);
  wolfSSL_dtls_get_current_timeout_ExpectAndReturn(conn.wolf_ssl, 10);
  TEST_ASSERT_EQUAL(0, call_counter);
  he_internal_update_timeout(&conn);
  TEST_ASSERT_EQUAL(1, call_counter);
  // Should still be 1 as the callback should not have been called
  // as the timer hasn't been cleared yet
  he_internal_update_timeout(&conn);
  TEST_ASSERT_EQUAL(1, call_counter);
  // Let's clear the timeout with a nudge
  int res1 = he_conn_nudge(&conn);
  TEST_ASSERT_EQUAL(HE_SUCCESS, res1);
  // Try the callback again
  he_internal_update_timeout(&conn);
  // Now it should have incremented
  TEST_ASSERT_EQUAL(2, call_counter);
}

void test_event_generation(void) {
  conn.event_cb = event_cb;
  he_internal_generate_event(&conn, HE_EVENT_FIRST_MESSAGE_RECEIVED);

  TEST_ASSERT_EQUAL(1, call_counter);
}

void test_event_generation_no_cb(void) {
  conn.event_cb = NULL;
  he_internal_generate_event(&conn, HE_EVENT_FIRST_MESSAGE_RECEIVED);

  TEST_ASSERT_EQUAL(0, call_counter);
}

void test_he_conn_generate_session_id(void) {
  uint64_t test_session = 0;
  wc_RNG_GenerateBlock_ExpectAndReturn(&conn.wolf_rng, (byte *)&test_session, sizeof(uint64_t), 0);

  int res = he_internal_generate_session_id(&conn, &test_session);

  TEST_ASSERT_EQUAL(HE_SUCCESS, res);
}

void test_he_conn_generate_session_id_error(void) {
  uint64_t test_session = 0;
  wc_RNG_GenerateBlock_ExpectAndReturn(&conn.wolf_rng, (byte *)&test_session, sizeof(uint64_t),
                                       FIXTURE_FATAL_ERROR);

  int res = he_internal_generate_session_id(&conn, &test_session);
  TEST_ASSERT_EQUAL(HE_ERR_RNG_FAILURE, res);
  TEST_ASSERT_EQUAL(0, test_session);
}

void test_he_conn_rotate_session_id_client(void) {
  uint64_t test_session = 0;

  conn.is_server = false;

  int res = he_conn_rotate_session_id(&conn, &test_session);

  TEST_ASSERT_EQUAL(HE_ERR_INVALID_CLIENT_STATE, res);
}

void test_he_conn_rotate_session_id_client_null_conn(void) {
  uint64_t test_session = 0;

  conn.is_server = false;

  int res = he_conn_rotate_session_id(NULL, &test_session);

  TEST_ASSERT_EQUAL(HE_ERR_NULL_POINTER, res);
}

void test_he_conn_rotate_session_id_already_pending(void) {
  uint64_t test_session = 0;
  conn.is_server = true;

  conn.pending_session_id = 0xdeadbeef;

  int res = he_conn_rotate_session_id(&conn, &test_session);

  TEST_ASSERT_EQUAL(HE_ERR_INVALID_CLIENT_STATE, res);
}

void test_he_conn_rotate_session_id_rng_failure(void) {
  uint64_t test_session = 0;
  conn.is_server = true;

  wc_RNG_GenerateBlock_ExpectAndReturn(&conn.wolf_rng, (byte *)&test_session, sizeof(uint64_t),
                                       FIXTURE_FATAL_ERROR);

  int res = he_conn_rotate_session_id(&conn, &test_session);

  TEST_ASSERT_EQUAL(HE_ERR_RNG_FAILURE, res);
}

int fixture_wc_RNG_GenerateBlock(RNG *rng, unsigned char *bytes, unsigned int size, int numCalls) {
  TEST_ASSERT_EQUAL(&conn.wolf_rng, rng);
  TEST_ASSERT_EQUAL(sizeof(uint64_t), size);

  uint64_t *number = (uint64_t *)bytes;
  *number = 0xdeadbeef;

  //(&conn.wolf_rng, (byte *)&test_session, sizeof(uint64_t), -1);
  return 0;
}

void test_he_conn_rotate_session_id(void) {
  uint64_t test_session = 0;
  conn.is_server = true;

  wc_RNG_GenerateBlock_Stub(fixture_wc_RNG_GenerateBlock);

  int res = he_conn_rotate_session_id(&conn, &test_session);

  TEST_ASSERT_EQUAL(HE_SUCCESS, res);
  TEST_ASSERT_EQUAL(0xdeadbeef, conn.pending_session_id);
  TEST_ASSERT_EQUAL(0xdeadbeef, test_session);
}

void test_he_conn_rotate_session_id_no_output(void) {
  conn.is_server = true;

  wc_RNG_GenerateBlock_Stub(fixture_wc_RNG_GenerateBlock);

  int res = he_conn_rotate_session_id(&conn, NULL);

  TEST_ASSERT_EQUAL(HE_SUCCESS, res);
  TEST_ASSERT_EQUAL(0xdeadbeef, conn.pending_session_id);
}

void test_he_conn_get_session_id(void) {
  uint64_t test_session = 0x00FF00FF00FF00FF;
  conn.session_id = test_session;

  uint64_t res = he_conn_get_session_id(&conn);
  TEST_ASSERT_EQUAL(test_session, res);
}

void test_he_conn_get_pending_session_id(void) {
  uint64_t test_session = 0x00FF00FF00FF00FF;
  conn.pending_session_id = test_session;

  uint64_t res = he_conn_get_pending_session_id(&conn);
  TEST_ASSERT_EQUAL(test_session, res);
}

void test_he_conn_set_session_id(void) {
  uint64_t test_session = 0x00FF00FF00FF00FF;

  // Check it's zero'd first
  TEST_ASSERT_EQUAL(0, conn.session_id);

  he_return_code_t res = he_conn_set_session_id(&conn, test_session);
  TEST_ASSERT_EQUAL(test_session, conn.session_id);
  TEST_ASSERT_EQUAL(HE_SUCCESS, res);
}

void test_he_conn_set_session_id_already_set(void) {
  uint64_t test_session = 0x00FF00FF00FF00FF;
  uint64_t init_session = 0xFFFFFFFF00000000;

  conn.session_id = init_session;

  // Check it's set first
  TEST_ASSERT_EQUAL(init_session, conn.session_id);

  he_return_code_t res = he_conn_set_session_id(&conn, test_session);
  TEST_ASSERT_EQUAL(init_session, conn.session_id);
  TEST_ASSERT_EQUAL(HE_ERR_INVALID_CLIENT_STATE, res);
}

void test_he_conn_set_session_id_null_check(void) {
  he_return_code_t res = he_conn_set_session_id(NULL, 0);
  TEST_ASSERT_EQUAL(HE_ERR_NULL_POINTER, res);
}

void test_he_conn_supports_renegotiation_is_null(void) {
  bool res = he_conn_supports_renegotiation(NULL);
  TEST_ASSERT_FALSE(res);
}

void test_he_conn_supports_renegotiation(void) {
  wolfSSL_SSL_get_secure_renegotiation_support_ExpectAndReturn(conn.wolf_ssl, true);
  bool res = he_conn_supports_renegotiation(&conn);
  TEST_ASSERT_TRUE(res);
}

void test_he_conn_set_protocol_version_null_conn(void) {
  he_return_code_t res = he_conn_set_protocol_version(NULL, 0x01, 0x01);
  TEST_ASSERT_EQUAL(HE_ERR_NULL_POINTER, res);
}

void test_he_conn_is_error_fatal_code_for_success(void) {
  bool res = he_conn_is_error_fatal(&conn, HE_SUCCESS);
  TEST_ASSERT_FALSE(res);
}

void test_he_conn_is_error_fatal_code_for_non_fatal_ssl(void) {
  bool res = he_conn_is_error_fatal(&conn, HE_ERR_SSL_ERROR_NONFATAL);
  TEST_ASSERT_FALSE(res);
}

void test_he_conn_is_error_fatal_code_for_null_pointer(void) {
  bool res = he_conn_is_error_fatal(&conn, HE_ERR_NULL_POINTER);
  TEST_ASSERT_TRUE(res);
}

void test_he_conn_is_valid_server_null_pointer_check(void) {
  he_return_code_t res = he_conn_is_valid_server(&ssl_ctx, NULL);
  TEST_ASSERT_EQUAL(HE_ERR_NULL_POINTER, res);
}

void test_he_conn_is_valid_server_no_mtu_set(void) {
  he_return_code_t res = he_conn_is_valid_server(&ssl_ctx, &conn);
  TEST_ASSERT_EQUAL(HE_ERR_CONF_MTU_NOT_SET, res);
}

void test_he_conn_is_valid_server_bad_protocol(void) {
  he_return_code_t res1 = he_conn_set_outside_mtu(&conn, 1500);
  TEST_ASSERT_EQUAL(HE_SUCCESS, res1);

  he_return_code_t res2 = he_conn_set_protocol_version(&conn, 0xFF, 0xFF);
  TEST_ASSERT_EQUAL(HE_SUCCESS, res2);

  he_return_code_t res3 = he_conn_is_valid_server(&ssl_ctx, &conn);
  TEST_ASSERT_EQUAL(HE_ERR_INCORRECT_PROTOCOL_VERSION, res3);
}

void test_he_conn_is_valid_server(void) {
  he_return_code_t res1 = he_conn_set_outside_mtu(&conn, 1500);
  TEST_ASSERT_EQUAL(HE_SUCCESS, res1);

  he_return_code_t res2 = he_conn_set_protocol_version(&conn, 0x00, 0x00);
  TEST_ASSERT_EQUAL(HE_SUCCESS, res2);

  he_return_code_t res3 = he_conn_is_valid_server(&ssl_ctx, &conn);
  TEST_ASSERT_EQUAL(HE_SUCCESS, res3);
}

void test_he_conn_schedule_renegotiation(void) {
  TEST_ASSERT_FALSE(conn.renegotiation_due);

  he_return_code_t res = he_conn_schedule_renegotiation(&conn);
  TEST_ASSERT_EQUAL(HE_SUCCESS, res);

  TEST_ASSERT_TRUE(conn.renegotiation_due);
}

void test_he_internal_renegotiate_ssl_not_online(void) {
  he_return_code_t res = he_internal_renegotiate_ssl(&conn);
  TEST_ASSERT_EQUAL(HE_SUCCESS, res);
}

void test_he_internal_renegotiate_ssl_already_renegotiating(void) {
  // Set our state online and set renegotiation in progress
  conn.state = HE_STATE_ONLINE;
  conn.renegotiation_in_progress = true;

  he_return_code_t res = he_internal_renegotiate_ssl(&conn);
  TEST_ASSERT_EQUAL(HE_SUCCESS, res);
}

void test_he_internal_renegotiate_dtls(void) {
  // Set our state online and set renegotiation in progress
  conn.state = HE_STATE_ONLINE;
  conn.renegotiation_in_progress = false;

  wolfSSL_SSL_get_secure_renegotiation_support_ExpectAndReturn(conn.wolf_ssl, true);
  wolfSSL_Rehandshake_ExpectAndReturn(conn.wolf_ssl, SSL_SUCCESS);

  he_return_code_t res = he_internal_renegotiate_ssl(&conn);
  TEST_ASSERT_EQUAL(HE_SUCCESS, res);
}

void test_he_internal_renegotiate_dtls_not_supported(void) {
  // Set our state online and set renegotiation in progress
  conn.state = HE_STATE_ONLINE;
  conn.renegotiation_in_progress = false;

  wolfSSL_SSL_get_secure_renegotiation_support_ExpectAndReturn(conn.wolf_ssl, false);

  he_return_code_t res = he_internal_renegotiate_ssl(&conn);
  TEST_ASSERT_EQUAL(HE_SUCCESS, res);
}

void test_he_internal_renegotiate_tls(void) {
  // Set our state online and set renegotiation in progress
  conn.state = HE_STATE_ONLINE;
  conn.renegotiation_in_progress = false;
  conn.connection_type = HE_CONNECTION_TYPE_STREAM;

  wolfSSL_SSL_get_secure_renegotiation_support_ExpectAndReturn(conn.wolf_ssl, false);
  wolfSSL_update_keys_ExpectAndReturn(conn.wolf_ssl, SSL_SUCCESS);

  he_return_code_t res = he_internal_renegotiate_ssl(&conn);
  TEST_ASSERT_EQUAL(HE_SUCCESS, res);
}

void test_he_internal_renegotiate_ssl_error_want_read(void) {
  // Set our state online and set renegotiation in progress
  conn.state = HE_STATE_ONLINE;
  conn.renegotiation_in_progress = false;

  wolfSSL_SSL_get_secure_renegotiation_support_ExpectAndReturn(conn.wolf_ssl, true);
  wolfSSL_Rehandshake_ExpectAndReturn(conn.wolf_ssl, SSL_FATAL_ERROR);
  wolfSSL_get_error_ExpectAndReturn(conn.wolf_ssl, SSL_FATAL_ERROR, SSL_ERROR_WANT_READ);
  wolfSSL_dtls_get_current_timeout_ExpectAndReturn(conn.wolf_ssl, 10);

  he_return_code_t res = he_internal_renegotiate_ssl(&conn);
  TEST_ASSERT_EQUAL(HE_SUCCESS, res);
}

void test_he_internal_renegotiate_ssl_error_want_write(void) {
  // Set our state online and set renegotiation in progress
  conn.state = HE_STATE_ONLINE;
  conn.renegotiation_in_progress = false;

  wolfSSL_SSL_get_secure_renegotiation_support_ExpectAndReturn(conn.wolf_ssl, true);
  wolfSSL_Rehandshake_ExpectAndReturn(conn.wolf_ssl, SSL_FATAL_ERROR);
  wolfSSL_get_error_ExpectAndReturn(conn.wolf_ssl, SSL_FATAL_ERROR, SSL_ERROR_WANT_WRITE);
  wolfSSL_dtls_get_current_timeout_ExpectAndReturn(conn.wolf_ssl, 10);

  he_return_code_t res = he_internal_renegotiate_ssl(&conn);
  TEST_ASSERT_EQUAL(HE_SUCCESS, res);
}

void test_he_internal_renegotiate_ssl_error_fatal(void) {
  // Set our state online and set renegotiation in progress
  conn.state = HE_STATE_ONLINE;
  conn.renegotiation_in_progress = false;

  wolfSSL_SSL_get_secure_renegotiation_support_ExpectAndReturn(conn.wolf_ssl, true);
  wolfSSL_Rehandshake_ExpectAndReturn(conn.wolf_ssl, SSL_FATAL_ERROR);
  wolfSSL_get_error_ExpectAndReturn(conn.wolf_ssl, SSL_FATAL_ERROR, SSL_FATAL_ERROR);

  he_return_code_t res = he_internal_renegotiate_ssl(&conn);
  TEST_ASSERT_EQUAL(HE_ERR_SSL_ERROR, res);
}

void test_he_conn_server_connect_null_pointers(void) {
  he_return_code_t res = he_conn_server_connect(NULL, NULL, NULL);
  TEST_ASSERT_EQUAL(HE_ERR_NULL_POINTER, res);
}

void test_he_conn_server_connect(void) {
  he_return_code_t res1 = he_conn_set_outside_mtu(&conn, 1500);
  TEST_ASSERT_EQUAL(HE_SUCCESS, res1);

  he_return_code_t res2 = he_conn_set_protocol_version(&conn, 0x00, 0x00);
  TEST_ASSERT_EQUAL(HE_SUCCESS, res2);

  wolfSSL_new_ExpectAndReturn(ssl_ctx.wolf_ctx, conn.wolf_ssl);
  wolfSSL_dtls_set_using_nonblock_Expect(conn.wolf_ssl, 1);
  wolfSSL_dtls_set_mtu_ExpectAndReturn(conn.wolf_ssl, 1425, SSL_SUCCESS);
  wolfSSL_SetIOWriteCtx_Expect(conn.wolf_ssl, &conn);
  wolfSSL_SetIOReadCtx_Expect(conn.wolf_ssl, &conn);

  wolfSSL_negotiate_ExpectAndReturn(conn.wolf_ssl, SSL_SUCCESS);
  wolfSSL_write_IgnoreAndReturn(SSL_SUCCESS);
  wolfSSL_dtls_get_current_timeout_ExpectAndReturn(conn.wolf_ssl, 10);
  wc_RNG_GenerateBlock_IgnoreAndReturn(0);

  he_return_code_t res = he_conn_server_connect(&conn, &ssl_ctx, NULL);
  TEST_ASSERT_EQUAL(HE_SUCCESS, res);
}

void test_he_conn_server_connect_dn_set(void) {
  he_return_code_t res3 = he_ssl_ctx_set_server_dn(&ssl_ctx, "testdn");
  TEST_ASSERT_EQUAL(HE_SUCCESS, res3);

  he_return_code_t res1 = he_conn_set_outside_mtu(&conn, 1500);
  TEST_ASSERT_EQUAL(HE_SUCCESS, res1);

  he_return_code_t res2 = he_conn_set_protocol_version(&conn, 0x00, 0x00);
  TEST_ASSERT_EQUAL(HE_SUCCESS, res2);

  wolfSSL_new_ExpectAndReturn(ssl_ctx.wolf_ctx, conn.wolf_ssl);
  wolfSSL_dtls_set_using_nonblock_Expect(conn.wolf_ssl, 1);
  wolfSSL_dtls_set_mtu_ExpectAndReturn(conn.wolf_ssl, 1425, SSL_SUCCESS);
  wolfSSL_SetIOWriteCtx_Expect(conn.wolf_ssl, &conn);
  wolfSSL_SetIOReadCtx_Expect(conn.wolf_ssl, &conn);
  wolfSSL_check_domain_name_ExpectAndReturn(conn.wolf_ssl, ssl_ctx.server_dn, SSL_SUCCESS);

  wolfSSL_negotiate_ExpectAndReturn(conn.wolf_ssl, SSL_SUCCESS);
  wolfSSL_write_IgnoreAndReturn(SSL_SUCCESS);
  wolfSSL_dtls_get_current_timeout_ExpectAndReturn(conn.wolf_ssl, 10);
  wc_RNG_GenerateBlock_IgnoreAndReturn(0);

  he_return_code_t res = he_conn_server_connect(&conn, &ssl_ctx, NULL);
  TEST_ASSERT_EQUAL(HE_SUCCESS, res);
}

void test_he_conn_server_connect_dn_set_fail(void) {
  he_return_code_t res3 = he_ssl_ctx_set_server_dn(&ssl_ctx, "testdn");
  TEST_ASSERT_EQUAL(HE_SUCCESS, res3);

  he_return_code_t res1 = he_conn_set_outside_mtu(&conn, 1500);
  TEST_ASSERT_EQUAL(HE_SUCCESS, res1);

  he_return_code_t res2 = he_conn_set_protocol_version(&conn, 0x00, 0x00);
  TEST_ASSERT_EQUAL(HE_SUCCESS, res2);

  wolfSSL_new_ExpectAndReturn(ssl_ctx.wolf_ctx, conn.wolf_ssl);
  wolfSSL_dtls_set_using_nonblock_Expect(conn.wolf_ssl, 1);
  wolfSSL_dtls_set_mtu_ExpectAndReturn(conn.wolf_ssl, 1425, SSL_SUCCESS);
  wolfSSL_SetIOWriteCtx_Expect(conn.wolf_ssl, &conn);
  wolfSSL_SetIOReadCtx_Expect(conn.wolf_ssl, &conn);
  wolfSSL_check_domain_name_ExpectAndReturn(conn.wolf_ssl, ssl_ctx.server_dn, SSL_FATAL_ERROR);

  he_return_code_t res = he_conn_server_connect(&conn, &ssl_ctx, NULL);
  TEST_ASSERT_EQUAL(HE_ERR_INIT_FAILED, res);
}

void test_he_internal_send_auth_bad_state(void) {
  conn.state = HE_STATE_ONLINE;
  he_return_code_t res = he_internal_send_auth(&conn);
  TEST_ASSERT_EQUAL(HE_ERR_INVALID_CLIENT_STATE, res);
}
