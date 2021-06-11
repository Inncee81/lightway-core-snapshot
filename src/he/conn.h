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

/**
 * @file conn.h
 * @brief Functions for managing the connection
 *
 */

#ifndef CONN_H
#define CONN_H

#include <he.h>

// Scale 1 second to wolf to 100ms
#define HE_WOLF_TIMEOUT_MULTIPLIER 100
#define HE_WOLF_RENEGOTIATION_TIMEOUT_MULTIPLIER 1000

/**
 * @brief Creates a Helium connection struct
 * @return he_conn_t* Returns a pointer to a valid Helium connection
 * @note This function allocates memory
 *
 * This function must be called to create the initial Helium connection
 * for use with other functions
 */
he_conn_t *he_conn_create(void);

/**
 * @brief Releases all memory allocate by Helium for this connection
 * @param conn A pointer to a valid Helium connection
 * @return HE_SUCCESS This function cannot fail
 *
 * It will first remove all of the callbacks which means no Helium callbacks will be triggered after
 * calling this function. It is thus an error to call any Helium functions on this connection after
 * it has been destroyed.
 */
he_return_code_t he_conn_destroy(he_conn_t *conn);

/**
 * @brief Checks whether the client conn has the basic values to allow Helium to connect.
 * @param ssl_ctx A pointer to a valid SSL context
 * @param conn A pointer to a valid connection conn
 * @return HE_ERR_NULL_POINTER The conn pointer supplied is NULL
 * @return HE_ERR_CONF_USERNAME_NOT_SET The username has not been set
 * @return HE_ERR_CONF_PASSWORD_NOT_SET The password has not been set
 * @return HE_ERR_CONF_MTU_NOT_SET The external MTU has not been set
 * @return HE_ERR_INCORRECT_PROTOCOL_VERSION The protocol version has been set
 * @return HE_SUCCESS The basic configuration options have been set
 *
 * @note These return codes are similar to `he_conn_client_connect` because that function will call
 *       this function before attempting to connect.
 */
he_return_code_t he_conn_is_valid_client(he_ssl_ctx_t *ssl_ctx, he_conn_t *conn);

/**
 * @brief Checks whether the client conn has the basic values to allow Helium to connect.
 * @param ssl_ctx A pointer to a valid SSL context
 * @param conn A pointer to a valid connection conn
 * @return HE_ERR_NULL_POINTER The conn pointer supplied is NULL
 * @return HE_ERR_CONF_MTU_NOT_SET The external MTU has not been set
 * @return HE_ERR_INCORRECT_PROTOCOL_VERSION The protocol version has been set to an unsupported
 * version
 * @return HE_SUCCESS The basic configuration options have been set
 *
 * @note These return codes are similar to `he_conn_server_connect` because that function will call
 *       this function before attempting to connect.
 */
he_return_code_t he_conn_is_valid_server(he_ssl_ctx_t *ssl_ctx, he_conn_t *conn);

/**
 * @brief Set the username to authenticate with
 * @param conn A pointer to a valid conn
 * @param username A pointer to the username
 * @return HE_SUCCESS Username has been set
 * @return HE_ERR_STRING_TOO_LONG Username is too long
 * @return HE_ERR_EMPTY_STRING String is empty
 */
int he_conn_set_username(he_conn_t *conn, const char *username);

/**
 * @brief Get the username that Helium will authenticate with, previously set by
 * he_client_set_username
 * @param conn A pointer to a valid connection
 * @return const char* A pointer to the username
 */
const char *he_conn_get_username(const he_conn_t *conn);

/**
 * @brief Check if the password has been set.
 * @param conn A pointer to a valid connection
 * @return bool Returns true or false depending on whether it has been configured
 *
 * It is anticipated that this feature will be used for implementing UIs that don't maintain their
 * own connection state.
 */
bool he_conn_is_username_set(const he_conn_t *conn);

/**
 * @brief Sets the password Helium should use to authenticate with
 * @param conn A pointer to a valid connection
 * @param password A pointer to the password
 * @return HE_SUCCESS The password has been set
 * @return HE_ERR_STRING_TOO_LONG Password is too long
 * @return HE_ERR_EMPTY_STRING String is empty
 * @note There is no he_client_get_password or equivalent for security reasons
 */
he_return_code_t he_conn_set_password(he_conn_t *conn, const char *password);

/**
 * @brief Check if the password has been set.
 * @param conn A pointer to a valid connection
 * @return bool Returns true or false depending on whether it has been configured
 *
 * It is anticipated that this feature will be used for implementing UIs that don't maintain their
 * own connection state.
 */
bool he_conn_is_password_set(const he_conn_t *conn);

/**
 * @brief Set the MTU for the outside transport mechanism. Usually this will be the MTU of the
 * devices Internet connection.
 * @param conn A pointer to a valid connection
 * @param mtu The MTU of the outside transport mechanism in bytes
 * @return HE_SUCCESS The MTU value was set
 * @note A default value is not set as although Ethernet is almost always 1500, mobile devices have
 * a wide range of options
 *
 * @caution Setting the MTU will update the MSS clamp size to an optimal value for the new MTU.
 */
int he_conn_set_outside_mtu(he_conn_t *conn, int mtu);

/**
 * @brief Get the MTU value for the outside transport mechanism.
 * *@param conn A pointer to a valid connection.
 * @ return int The MTU value in bytes.
 */
int he_conn_get_outside_mtu(he_conn_t *conn);

/**
 * @brief Check if the outside MTU has been set.
 * @param conn A pointer to a valid connection
 * @return bool Returns true or false depending on whether it has been configured
 *
 * It is anticipated that this feature will be used for implementing UIs that don't maintain their
 * own connection state.
 */
bool he_conn_is_outside_mtu_set(he_conn_t *conn);

/**
 * @brief Store a pointer in the context that will be made available in all Helium callbacks
 * @param conn A valid connection
 * @param data A pointer to a user provided data structure
 *
 * Helium will interact with the host application primarily through the use of callback functions.
 * To avoid the need for global variables, a pointer to some data structure can be set on the
 * context. This context will be passed in all function callbacks as void pointer.
 */
he_return_code_t he_conn_set_context(he_conn_t *conn, void *data);

/**
 * @brief Retrieve the pointer to the user supplied context.
 * @param conn A pointer to a valid HE context
 * @return void* The void pointer that was set previously or NULL if none was set
 */
void *he_conn_get_context(he_conn_t *conn);
he_return_code_t he_internal_conn_configure(he_conn_t *conn, he_ssl_ctx_t *ctx);

/**
 * @brief Tries to establish a connection with a Helium server
 * @param conn A pointer to a valid Helium connection
 * @param conn A pointer to a valid connection conn
 * @param ssl_ctx A pointer to valid and started SSL context
 * @param ctx_conn A pointer to a valid SSL context connection
 * @param plugins A pointer to a valid plugin chain
 * @return HE_ERR_NULL_POINTER One or more required pointers are NULL
 * @return HE_ERR_CONF_USERNAME_NOT_SET The username has not been set
 * @return HE_ERR_CONF_PASSWORD_NOT_SET The password has not been set
 * @return HE_ERR_CONF_MTU_NOT_SET The external MTU has not been set
 * @return HE_ERR_INIT_FAILED Helium was unable to initialise itself
 * @return HE_ERR_SSL_CERT Generic failure in the SSL engine
 * @return HE_ERR_CONNECT_FAILED There was an I/O issue trying to connect to the server.
 * @return HE_SUCCESS Helium is in the process of connecting
 *
 * @note This function triggers the initialisation and initial connection to a Helium server.
 * However it is asynchronous, Helium is *not* connected when this function returns, merely that the
 * connection is in progress. Use event and state change callbacks to determine the actual state of
 * Helium
 *
 * This function has a lot of return codes as it is where Helium tries to apply and configure the
 * crypto engine. All of the return codes except for HE_SUCCESS are effectively fatal errors. Trying
 * to call *he_conn_client_connect* again without changing the connection is unlikely to succeed.
 */
he_return_code_t he_conn_client_connect(he_conn_t *conn, he_ssl_ctx_t *ssl_ctx,
                                        he_plugin_chain_t *plugins);

/**
 * @brief Tries to establish a connection with a Helium client
 * @param conn A pointer to a valid Helium connection
 * @param ssl_ctx A pointer to valid and started SSL context
 * @param ctx_conn A pointer to a valid SSL context connection
 * @param plugins A pointer to a valid plugin chain
 * @return HE_ERR_NULL_POINTER One or more required pointers are NULL
 * @return HE_ERR_INIT_FAILED Helium was unable to initialise itself
 * @return HE_ERR_SSL_CERT Generic failure in the SSL engine
 * @return HE_ERR_CONNECT_FAILED There was an I/O issue trying to connect to the server.
 * @return HE_SUCCESS Helium is in the process of connecting
 *
 * @note This function triggers the initialisation and initial connection to a Helium client.
 * However it is asynchronous, Helium is *not* connected when this function returns, merely that the
 * connection is in progress. Use event and state change callbacks to determine the actual state of
 * Helium
 *
 * This function has a lot of return codes as it is where Helium tries to apply and configure the
 * crypto engine. All of the return codes except for HE_SUCCESS are effectively fatal errors. Trying
 * to call *he_conn_client_connect* again without changing the connection is unlikely to succeed.
 */
he_return_code_t he_conn_server_connect(he_conn_t *conn, he_ssl_ctx_t *ssl_ctx,
                                        he_plugin_chain_t *plugins);

/**
 * @brief Try to cleanly disconnect from the remote Helium instance (client or server).
 * @return HE_ERR_NEVER_CONNECTED The connection has never been connected and so cannot be
 * disconnected. It is safe to destroy the client state here.
 * @return HE_ERR_INVALID_CLIENT_STATE This function should only be used when Helium is in the
 * online state. It is safe to destroy the client in other states.
 * @return HE_SUCCESS The disconnect process has started
 *
 * @note Like he_client_connect, this is an asynchronous process. Watch state changes to determine
 * when Helium has actually disconnected
 * @note This function is not yet well described and is likely to change
 */
he_return_code_t he_conn_disconnect(he_conn_t *conn);

void he_internal_disconnect_and_shutdown(he_conn_t *conn);

void he_internal_change_conn_state(he_conn_t *conn, he_client_state_t state);

/**
 * @brief Sends a message over the secured tunnel
 * @param conn A pointer to a valid connection
 * @param message A pointer to the raw message to be sent
 * @param length The length of the message
 * @return he_client_return_code_t HE_SUCCESS
 */
he_return_code_t he_internal_send_message(he_conn_t *conn, uint8_t *message, uint16_t length);
he_return_code_t he_internal_send_goodbye(he_conn_t *conn);
he_return_code_t he_internal_send_auth(he_conn_t *conn);

/**
 * @brief Tell Helium to send a keepalive message. This can be used to avoid NAT timing out.
 * @param conn A pointer to a valid connection
 * @return HE_SUCCESS Keep alive was sent
 *
 * This is used to send a heartbeat message to a Helium server and get a reply. Its prmary use is to
 * ensure that the connection doesn't time out due to NAT traversal. This feature is not mandatory,
 * but as Helium cannot by itself know when to send these, it is up to the host application to call
 * this function at the required intervals.
 */
he_return_code_t he_conn_send_keepalive(he_conn_t *conn);

/**
 * @brief Tell Helium to schedule a renegotiation
 * @param conn A pointer to a valid connection
 * @return HE_SUCCESS A renegotiation is scheduled.
 *
 * Note that this schedules a key rotation or D/TLS renegotiation on the next time we process
 * data on this connection; calling this function provides *no* time-based guarantees, only that
 * when we see this client again we will renegotiate.
 */
he_return_code_t he_conn_schedule_renegotiation(he_conn_t *conn);
he_return_code_t he_internal_renegotiate_ssl(he_conn_t *conn);

/**
 * @brief Updates the timeout for a connectionand triggers the timeout callback if set
 * @param conn A pointer to a valid connection
 */
void he_internal_update_timeout(he_conn_t *conn);

/**
 * @brief Returns the number of miliseconds that host application should wait before nudging Helium
 * @param conn A pointer to a valid connection
 * @return The number of miliseconds to wait before nudging again
 *
 * @warning This value is updated after Helium completes a read cycle. It should be called directly
 * after those functions
 */
int he_conn_get_nudge_time(he_conn_t *conn);

/**
 * @brief Nudges Helium
 * @param conn A pointer to a valid connection
 * @return HE_SUCCESS The nudge was successful
 * @return HE_CONNECTION_TIMED_OUT Nudge timed out
 */
he_return_code_t he_conn_nudge(he_conn_t *conn);

/**
 * @brief Returns the state that the client is currently in
 * @param conn A pointer to a valid connection
 * @return he_client_state_t The state the connection is in
 * @see he_set_state_change_cb
 * @see he_client_state_t
 */
he_client_state_t he_conn_get_state(he_conn_t *conn);

/**
 * @brief Returns the session ID for this connection
 */
uint64_t he_conn_get_session_id(he_conn_t *conn);

/**
 * @brief Sets the session ID for this connection
 */
he_return_code_t he_conn_set_session_id(he_conn_t *conn, uint64_t session_id);

/**
 * @brief Returns the pending session ID for this connection, if there is one
 * @param conn A pointer to a valid connection
 * @return 0 If there is no pending session
 * @note On a client connection this function will always return 0; there are no pending client
 * session IDs
 */
uint64_t he_conn_get_pending_session_id(he_conn_t *conn);

/**
 * @brief Returns true if a given error (a return from libhelium where he_return_code_t !=
 * HE_SUCCESS) is fatal and should terminate the connection.
 */
bool he_conn_is_error_fatal(he_conn_t *conn, he_return_code_t error_msg);

/**
 * @brief Rotate the session ID for this connection
 *
 * After calling this function, the value returned by he_conn_get_session_id will NOT be updated.
 * The value returned by he_conn_get_pending_session_id is guaranteed to be the same as the value
 * pointed to by new_session_id if that parameter is not null.
 *
 * After the event "HE_EVENT_PENDING_SESSION_ACKNOWLEDGED" fires, he_conn_get_pending_session_id
 * will return 0, and the value for he_conn_get_session_id will be the same as the session ID
 * generated by this call.
 *
 * @param conn A pointer to a valid connection
 * @param[out] A uint64_t pointer; if not null the value will be updated with the new session ID
 * @return HE_ERR_INVALID_CLIENT_STATE If this connection has not been started, or is a client
 * connection, or there is already a pending session rotation
 * @return HE_ERR_RNG_FAILURE if we were unable to generate a random number
 * @return HE_SUCCESS Session ID rotation begun
 */
he_return_code_t he_conn_rotate_session_id(he_conn_t *conn, uint64_t *new_session_id);

/**
 * @brief Whether this particular connection supports renegoation
 *
 * Only pre-Dec 2020 clients won't support this.
 */
bool he_conn_supports_renegotiation(he_conn_t *conn);

void he_internal_generate_event(he_conn_t *conn, he_client_event_t event);

/**
 * @brief On the server, sets the major/minor version number for this connection
 * @param conn A pointer to a valid server connection
 * @param major_version
 * @param minor_version
 * @return HE_SUCCESS if the major/minor version was set successfully
 * @note It will result in an invalid state if the protocol version is set on a connection that is
 * later used as a client connection
 */
he_return_code_t he_conn_set_protocol_version(he_conn_t *conn, uint8_t major_version,
                                              uint8_t minor_version);

size_t he_internal_calculate_data_packet_length(he_conn_t *conn, size_t length);

/**
 * @brief Generate a random session ID
 * @param context A pointer to a valid SSL server context
 * @param [out] session_id_out A pointer to a uint64_t, where we will write the session ID
 * @return HE_ERR_RNG_FAILURE An error occurred generating the session ID
 * @return HE_SUCCESS Random value generated correctly
 */
he_return_code_t he_internal_generate_session_id(he_conn_t *conn, uint64_t *session_id_out);

#endif  // CONN_H
