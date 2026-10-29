# Firmware Update System API Documentation

## Table of Contents

1. [Authentication API](#authentication-api)
2. [Device API](#device-api)
3. [Firmware API](#firmware-api)
4. [Batch API](#batch-api)
5. [OTA API](#ota-api)
6. [Logs API](#logs-api)
7. [Error Handling](#error-handling)
8. [Authentication](#authentication)

## Base URL

All API endpoints are relative to the base URL: `http://your-server:8000`

## Authentication

Most endpoints require authentication using JWT tokens. To authenticate:

1. Obtain a token by sending a POST request to `/api/auth/token`
2. Include the token in the Authorization header of subsequent requests:
   ```
   Authorization: Bearer <your_token>
   ```

## Authentication API

### Login for Access Token

**Endpoint**: `POST /api/auth/token`

**Description**: Authenticates an admin user and returns a JWT token

**Request Body**:
```json
{
  "username": "string",
  "password": "string"
}
```

**Response**:
```json
{
  "access_token": "string",
  "token_type": "bearer"
}
```

**Status Codes**:
- 200: Successful authentication
- 401: Invalid credentials

### Admin User Management

#### Create Admin User

**Endpoint**: `POST /api/auth/admins`

**Description**: Creates a new admin user

**Authorization**: Requires admin role

**Request Body**:
```json
{
  "username": "string",
  "email": "string",
  "password": "string",
  "role": "admin|manager|viewer"
}
```

**Response**:
```json
{
  "id": "integer",
  "username": "string",
  "email": "string",
  "role": "string",
  "is_active": "boolean",
  "last_login": "datetime|null",
  "created_at": "datetime",
  "updated_at": "datetime"
}
```

**Status Codes**:
- 200: Admin created successfully
- 400: Username already exists
- 401: Unauthorized

#### List Admin Users

**Endpoint**: `GET /api/auth/admins`

**Description**: Lists all admin users

**Authorization**: Requires admin role

**Query Parameters**:
- `skip`: Number of records to skip (default: 0)
- `limit`: Maximum number of records to return (default: 100)

**Response**:
```json
[
  {
    "id": "integer",
    "username": "string",
    "email": "string",
    "role": "string",
    "is_active": "boolean",
    "last_login": "datetime|null",
    "created_at": "datetime",
    "updated_at": "datetime"
  }
]
```

**Status Codes**:
- 200: Success
- 401: Unauthorized

#### Get Admin User

**Endpoint**: `GET /api/auth/admins/{admin_id}`

**Description**: Gets a specific admin user by ID

**Authorization**: Requires admin role

**Response**:
```json
{
  "id": "integer",
  "username": "string",
  "email": "string",
  "role": "string",
  "is_active": "boolean",
  "last_login": "datetime|null",
  "created_at": "datetime",
  "updated_at": "datetime"
}
```

**Status Codes**:
- 200: Success
- 401: Unauthorized
- 404: Admin not found

#### Update Admin User

**Endpoint**: `PUT /api/auth/admins/{admin_id}`

**Description**: Updates an admin user

**Authorization**: Requires admin role

**Request Body**:
```json
{
  "email": "string",
  "role": "admin|manager|viewer",
  "is_active": "boolean",
  "password": "string"
}
```

**Response**:
```json
{
  "id": "integer",
  "username": "string",
  "email": "string",
  "role": "string",
  "is_active": "boolean",
  "last_login": "datetime|null",
  "created_at": "datetime",
  "updated_at": "datetime"
}
```

**Status Codes**:
- 200: Success
- 401: Unauthorized
- 404: Admin not found

#### Delete Admin User

**Endpoint**: `DELETE /api/auth/admins/{admin_id}`

**Description**: Deletes an admin user

**Authorization**: Requires admin role

**Response**:
```json
{
  "status": "success"
}
```

**Status Codes**:
- 200: Success
- 400: Cannot delete yourself
- 401: Unauthorized
- 404: Admin not found

## Device API

### Register Device

**Endpoint**: `POST /api/devices/register`

**Description**: Registers a new device in the system

**Request Body**:
```json
{
  "mac_address": "string",
  "current_firmware_version": "string",
  "batch_id": "integer|null",
  "status": "string"
}
```

**Response**:
```json
{
  "id": "integer",
  "mac_address": "string",
  "current_firmware_version": "string",
  "batch_id": "integer|null",
  "status": "string",
  "last_check_time": "datetime",
  "last_update_time": "datetime",
  "created_at": "datetime",
  "updated_at": "datetime"
}
```

**Status Codes**:
- 201: Device created successfully
- 400: Device with this MAC address already exists

### List Devices

**Endpoint**: `GET /api/devices/`

**Description**: Lists all devices

**Authorization**: Requires admin, manager, or viewer role

**Query Parameters**:
- `skip`: Number of records to skip (default: 0)
- `limit`: Maximum number of records to return (default: 100)
- `batch_id`: Filter by batch ID (optional)

**Response**:
```json
[
  {
    "id": "integer",
    "mac_address": "string",
    "current_firmware_version": "string",
    "batch_id": "integer|null",
    "status": "string",
    "last_check_time": "datetime",
    "last_update_time": "datetime",
    "created_at": "datetime",
    "updated_at": "datetime"
  }
]
```

**Status Codes**:
- 200: Success
- 401: Unauthorized

### Get Device

**Endpoint**: `GET /api/devices/{device_id}`

**Description**: Gets a specific device by ID

**Authorization**: Requires admin, manager, or viewer role

**Response**:
```json
{
  "id": "integer",
  "mac_address": "string",
  "current_firmware_version": "string",
  "batch_id": "integer|null",
  "status": "string",
  "last_check_time": "datetime",
  "last_update_time": "datetime",
  "created_at": "datetime",
  "updated_at": "datetime"
}
```

**Status Codes**:
- 200: Success
- 401: Unauthorized
- 404: Device not found

### Update Device

**Endpoint**: `PUT /api/devices/{device_id}`

**Description**: Updates a device

**Authorization**: Requires admin or manager role

**Request Body**:
```json
{
  "batch_id": "integer|null",
  "status": "string"
}
```

**Response**:
```json
{
  "id": "integer",
  "mac_address": "string",
  "current_firmware_version": "string",
  "batch_id": "integer|null",
  "status": "string",
  "last_check_time": "datetime",
  "last_update_time": "datetime",
  "created_at": "datetime",
  "updated_at": "datetime"
}
```

**Status Codes**:
- 200: Success
- 401: Unauthorized
- 404: Device not found

### Delete Device

**Endpoint**: `DELETE /api/devices/{device_id}`

**Description**: Deletes a device

**Authorization**: Requires admin or manager role

**Status Codes**:
- 204: Success, no content
- 401: Unauthorized
- 404: Device not found

## Firmware API

### Upload Firmware

**Endpoint**: `POST /api/firmware/`

**Description**: Uploads a new firmware version

**Authorization**: Requires admin or manager role

**Request Body**: Multipart form data
- `version`: Firmware version string
- `update_name`: Name of the update
- `release_notes`: Release notes (optional)
- `is_beta`: Whether this is a beta release (default: false)
- `file`: Binary firmware file

**Response**:
```json
{
  "id": "integer",
  "version": "string",
  "update_name": "string",
  "release_notes": "string",
  "file_path": "string",
  "file_size": "integer",
  "checksum": "string",
  "is_beta": "boolean",
  "created_at": "datetime",
  "updated_at": "datetime"
}
```

**Status Codes**:
- 201: Firmware created successfully
- 400: Firmware version already exists
- 401: Unauthorized

### List Firmware

**Endpoint**: `GET /api/firmware/`

**Description**: Lists all firmware versions

**Authorization**: Requires admin, manager, or viewer role

**Query Parameters**:
- `skip`: Number of records to skip (default: 0)
- `limit`: Maximum number of records to return (default: 100)
- `include_beta`: Whether to include beta versions (default: false)

**Response**:
```json
[
  {
    "id": "integer",
    "version": "string",
    "update_name": "string",
    "release_notes": "string",
    "file_path": "string",
    "file_size": "integer",
    "checksum": "string",
    "is_beta": "boolean",
    "created_at": "datetime",
    "updated_at": "datetime"
  }
]
```

**Status Codes**:
- 200: Success
- 401: Unauthorized

### Get Firmware

**Endpoint**: `GET /api/firmware/{firmware_id}`

**Description**: Gets a specific firmware by ID

**Authorization**: Requires admin, manager, or viewer role

**Response**:
```json
{
  "id": "integer",
  "version": "string",
  "update_name": "string",
  "release_notes": "string",
  "file_path": "string",
  "file_size": "integer",
  "checksum": "string",
  "is_beta": "boolean",
  "created_at": "datetime",
  "updated_at": "datetime"
}
```

**Status Codes**:
- 200: Success
- 401: Unauthorized
- 404: Firmware not found

### Update Firmware

**Endpoint**: `PUT /api/firmware/{firmware_id}`

**Description**: Updates firmware metadata

**Authorization**: Requires admin or manager role

**Request Body**:
```json
{
  "update_name": "string",
  "release_notes": "string",
  "is_beta": "boolean"
}
```

**Response**:
```json
{
  "id": "integer",
  "version": "string",
  "update_name": "string",
  "release_notes": "string",
  "file_path": "string",
  "file_size": "integer",
  "checksum": "string",
  "is_beta": "boolean",
  "created_at": "datetime",
  "updated_at": "datetime"
}
```

**Status Codes**:
- 200: Success
- 401: Unauthorized
- 404: Firmware not found

### Download Firmware

**Endpoint**: `GET /api/firmware/{firmware_id}/download`

**Description**: Downloads the firmware binary file

**Response**: Binary file

**Status Codes**:
- 200: Success
- 404: Firmware not found or file not found

### Delete Firmware

**Endpoint**: `DELETE /api/firmware/{firmware_id}`

**Description**: Deletes a firmware version

**Authorization**: Requires admin role

**Status Codes**:
- 204: Success, no content
- 401: Unauthorized
- 404: Firmware not found

## Batch API

### Create Batch

**Endpoint**: `POST /api/batches/`

**Description**: Creates a new batch

**Authorization**: Requires admin or manager role

**Request Body**:
```json
{
  "name": "string",
  "description": "string",
  "check_update_time": "datetime",
  "auto_update": "boolean"
}
```

**Response**:
```json
{
  "id": "integer",
  "name": "string",
  "description": "string",
  "check_update_time": "datetime",
  "auto_update": "boolean",
  "update_state": "string",
  "last_update_check": "datetime|null",
  "last_update_trigger": "datetime|null",
  "devices_total": "integer",
  "devices_updated": "integer",
  "devices_failed": "integer",
  "created_at": "datetime",
  "updated_at": "datetime"
}
```

**Status Codes**:
- 201: Batch created successfully
- 401: Unauthorized

### List Batches

**Endpoint**: `GET /api/batches/`

**Description**: Lists all batches

**Authorization**: Requires admin, manager, or viewer role

**Query Parameters**:
- `skip`: Number of records to skip (default: 0)
- `limit`: Maximum number of records to return (default: 100)

**Response**:
```json
[
  {
    "id": "integer",
    "name": "string",
    "description": "string",
    "check_update_time": "datetime",
    "auto_update": "boolean",
    "update_state": "string",
    "last_update_check": "datetime|null",
    "last_update_trigger": "datetime|null",
    "devices_total": "integer",
    "devices_updated": "integer",
    "devices_failed": "integer",
    "created_at": "datetime",
    "updated_at": "datetime"
  }
]
```

**Status Codes**:
- 200: Success
- 401: Unauthorized

### Get Batch

**Endpoint**: `GET /api/batches/{batch_id}`

**Description**: Gets a specific batch by ID

**Authorization**: Requires admin, manager, or viewer role

**Response**:
```json
{
  "id": "integer",
  "name": "string",
  "description": "string",
  "check_update_time": "datetime",
  "auto_update": "boolean",
  "update_state": "string",
  "last_update_check": "datetime|null",
  "last_update_trigger": "datetime|null",
  "devices_total": "integer",
  "devices_updated": "integer",
  "devices_failed": "integer",
  "created_at": "datetime",
  "updated_at": "datetime"
}
```

**Status Codes**:
- 200: Success
- 401: Unauthorized
- 404: Batch not found

### Update Batch

**Endpoint**: `PUT /api/batches/{batch_id}`

**Description**: Updates a batch

**Authorization**: Requires admin or manager role

**Request Body**:
```json
{
  "name": "string",
  "description": "string",
  "check_update_time": "datetime",
  "auto_update": "boolean"
}
```

**Response**:
```json
{
  "id": "integer",
  "name": "string",
  "description": "string",
  "check_update_time": "datetime",
  "auto_update": "boolean",
  "update_state": "string",
  "last_update_check": "datetime|null",
  "last_update_trigger": "datetime|null",
  "devices_total": "integer",
  "devices_updated": "integer",
  "devices_failed": "integer",
  "created_at": "datetime",
  "updated_at": "datetime"
}
```

**Status Codes**:
- 200: Success
- 401: Unauthorized
- 404: Batch not found

### Delete Batch

**Endpoint**: `DELETE /api/batches/{batch_id}`

**Description**: Deletes a batch

**Authorization**: Requires admin role

**Status Codes**:
- 204: Success, no content
- 401: Unauthorized
- 404: Batch not found

### Assign Firmware to Batch

**Endpoint**: `POST /api/batches/{batch_id}/firmware`

**Description**: Assigns a firmware version to a batch

**Authorization**: Requires admin or manager role

**Request Body**:
```json
{
  "firmware_id": "integer",
  "is_active": "boolean"
}
```

**Response**:
```json
{
  "status": "success"
}
```

**Status Codes**:
- 200: Success
- 400: Could not assign firmware
- 401: Unauthorized
- 404: Batch not found

### Get Batch Devices

**Endpoint**: `GET /api/batches/{batch_id}/devices`

**Description**: Gets all devices in a batch

**Authorization**: Requires admin, manager, or viewer role

**Query Parameters**:
- `skip`: Number of records to skip (default: 0)
- `limit`: Maximum number of records to return (default: 100)

**Response**:
```json
[
  {
    "id": "integer",
    "mac_address": "string",
    "current_firmware_version": "string",
    "batch_id": "integer",
    "status": "string",
    "last_check_time": "datetime",
    "last_update_time": "datetime",
    "created_at": "datetime",
    "updated_at": "datetime"
  }
]
```

**Status Codes**:
- 200: Success
- 401: Unauthorized
- 404: Batch not found

### Trigger Batch Update

**Endpoint**: `POST /api/batches/{batch_id}/trigger-update`

**Description**: Triggers an update for all devices in a batch

**Authorization**: Requires admin or manager role

**Request Body**:
```json
{
  "force": "boolean"
}
```

**Response**:
```json
{
  "status": "update_triggered",
  "batch_state": "string",
  "devices_total": "integer"
}
```

**Status Codes**:
- 200: Success
- 400: Batch is already updating
- 401: Unauthorized
- 404: Batch not found

## OTA API

### Check for Update

**Endpoint**: `GET /api/ota/check-update`

**Description**: Checks if an update is available for a device

**Query Parameters**:
- `mac_address`: Device MAC address
- `current_version`: Current firmware version

**Response**:
```json
{
  "update_available": "boolean",
  "firmware_id": "integer",
  "version": "string",
  "update_name": "string",
  "release_notes": "string",
  "checksum": "string",
  "download_url": "string"
}
```

**Status Codes**:
- 200: Success
- 404: Device not found

### Report Update Status

**Endpoint**: `POST /api/ota/update-status`

**Description**: Reports the status of a firmware update

**Request Body**:
```json
{
  "mac_address": "string",
  "firmware_id": "integer",
  "status": "string",
  "error_message": "string|null"
}
```

**Response**:
```json
{
  "status": "success"
}
```

**Status Codes**:
- 200: Success
- 404: Device or firmware not found

## Logs API

### Get Device Logs

**Endpoint**: `GET /api/logs/device-logs`

**Description**: Gets logs for device updates

**Authorization**: Requires admin, manager, or viewer role

**Query Parameters**:
- `device_id`: Filter by device ID (optional)
- `start_date`: Filter by start date (optional)
- `end_date`: Filter by end date (optional)
- `level`: Filter by log level (optional)
- `skip`: Number of records to skip (default: 0)
- `limit`: Maximum number of records to return (default: 100)

**Response**:
```json
[
  {
    "id": "integer",
    "device_id": "integer",
    "firmware_id": "integer",
    "status": "string",
    "error_message": "string|null",
    "level": "string",
    "created_at": "datetime"
  }
]
```

**Status Codes**:
- 200: Success
- 401: Unauthorized

### Get Admin Logs

**Endpoint**: `GET /api/logs/admin-logs`

**Description**: Gets logs for administrative actions

**Authorization**: Requires admin role

**Query Parameters**:
- `start_date`: Filter by start date (optional)
- `end_date`: Filter by end date (optional)
- `level`: Filter by log level (optional)
- `entity_type`: Filter by entity type (optional)
- `entity_id`: Filter by entity ID (optional)
- `action_type`: Filter by action type (optional)
- `skip`: Number of records to skip (default: 0)
- `limit`: Maximum number of records to return (default: 100)

**Response**:
```json
[
  {
    "id": "integer",
    "admin_id": "integer",
    "action_type": "string",
    "entity_type": "string",
    "entity_id": "integer",
    "details": "string|null",
    "level": "string",
    "created_at": "datetime"
  }
]
```

**Status Codes**:
- 200: Success
- 401: Unauthorized

## Error Handling

The API uses standard HTTP status codes to indicate the success or failure of a request. In case of an error, the response body will contain a JSON object with details about the error:

```json
{
  "detail": "Error message"
}
```

### Common Error Codes

- **400 Bad Request**: The request was invalid or cannot be served
- **401 Unauthorized**: Authentication is required or failed
- **403 Forbidden**: The authenticated user doesn't have permission
- **404 Not Found**: The requested resource doesn't exist
- **409 Conflict**: The request conflicts with the current state
- **422 Unprocessable Entity**: The request was well-formed but contains semantic errors
- **500 Internal Server Error**: An error occurred on the server










# ESP32 OTA Interaction Guide with FastAPI Backend

This guide explains how an ESP32 device should interact with the provided FastAPI backend to perform Over-the-Air (OTA) firmware updates.

## Prerequisites (ESP32 Side)

1.  **WiFi Connectivity:** The ESP32 must be able to connect to a WiFi network.
2.  **HTTP(S) Client:** A library to make HTTP(S) requests (e.g., `esp_http_client` in ESP-IDF, `HTTPClient` or `WiFiClientSecure` in Arduino). Using HTTPS is **highly recommended**.
3.  **JSON Parsing:** A library to parse JSON responses from the server (e.g., `cJSON` in ESP-IDF, `ArduinoJson` in Arduino).
4.  **OTA Library:** The ESP32's OTA update library (e.g., `esp_https_ota` in ESP-IDF, `Update` class in Arduino).
5.  **Device Information:** The device needs to know its unique MAC address and its current firmware version string.

## Core OTA Workflow

The typical flow for an ESP32 device checking for and performing an update is as follows:

### 1. Initialization & Connection

*   Connect to the configured WiFi network.
*   Obtain the device's MAC address.
*   Retrieve the current firmware version (this is often hardcoded during compilation or stored in Non-Volatile Storage - NVS).

### 2. (Optional but Recommended) Device Registration

If the device isn't guaranteed to be pre-registered, it should attempt to register itself upon first boot or if update checks fail due to "Device not found".

*   **Endpoint:** `POST /api/devices/register`
*   **Method:** `POST`
*   **Request Body (JSON):**
    ```json
    {
      "mac_address": "AA:BB:CC:DD:EE:FF",
      "current_firmware_version": "1.0.0",
      "batch_id": null // Or the pre-assigned batch ID if known
    }
    ```
*   **Response:** If successful (HTTP 201), the server returns device details. If the MAC already exists (HTTP 400), the device can assume it's registered.

### 3. Checking for Updates

Periodically (e.g., every few hours or on startup), the device should check if a new firmware version is available for its batch.

*   **Endpoint:** `GET /api/ota/check-update`
*   **Method:** `GET`
*   **Query Parameters:**
    *   `mac_address`: The device's MAC address (e.g., `AA:BB:CC:DD:EE:FF`).
    *   `current_version`: The device's current firmware version string (e.g., `1.0.0`).
*   **Example URL:** `http://your_server_address/api/ota/check-update?mac_address=AA:BB:CC:DD:EE:FF¤t_version=1.0.0`
*   **Expected Response (JSON):**
    *   **If no update or device not found:**
        ```json
        // HTTP 200 if no update available for the current version
        { "update_available": false }

        // HTTP 404 if device MAC not registered
        { "detail": "Device not found" }
        ```
    *   **If update available (HTTP 200):**
        ```json
        {
          "update_available": true,
          "firmware_id": 5, // The ID of the new firmware
          "version": "1.1.0", // The version string of the new firmware
          "size": 123456, // Size of the firmware binary in bytes
          "checksum": "a1b2c3d4..." // SHA-256 checksum of the firmware binary
        }
        ```

### 4. Downloading Firmware (If Update Available)

If `update_available` is `true`, the device proceeds to download the new firmware.

*   **Endpoint:** `GET /api/firmware/{firmware_id}/download`
*   **Method:** `GET`
*   **Path Parameter:**
    *   `firmware_id`: The `firmware_id` received from the `/check-update` response.
*   **Example URL:** `http://your_server_address/api/firmware/5/download`
*   **Process:**
    1.  Use the ESP32's OTA library (e.g., `esp_https_ota` or `Update`).
    2.  Provide the download URL to the library.
    3.  The library will typically handle:
        *   Connecting to the server.
        *   Writing the downloaded binary stream to the next available OTA partition.
        *   Basic validation (e.g., partition writing).
    4.  **Crucially:** After the download *appears* complete via the library, you **must** validate it *before* setting the boot partition.
        *   **Size Validation:** Compare the actual bytes written/received with the `size` provided by the `/check-update` endpoint.
        *   **Checksum Validation:** Calculate the SHA-256 checksum of the downloaded binary in the OTA partition and compare it with the `checksum` provided by the `/check-update` endpoint. Libraries like `mbedtls` (included in ESP-IDF) can be used for this.

### 5. Applying the Update

*   **If Download & Validation Succeed:**
    1.  Use the OTA library to set the newly written partition as the boot partition (e.g., `esp_ota_set_boot_partition`).
    2.  Report the success status back to the server (See Step 6).
    3.  Restart the device (`esp_restart()`). The ESP32 bootloader will handle booting from the new partition.
*   **If Download or Validation Fails:**
    1.  Report the failure status back to the server, including an error message (See Step 6).
    2.  Do **not** set the boot partition.
    3.  Do **not** restart (unless desired for retry logic). The device continues running the old firmware.

### 6. Reporting Update Status

After attempting an update (whether successful or failed), the device should report the outcome back to the server.

*   **Endpoint:** `POST /api/ota/update-status`
*   **Method:** `POST`
*   **Request Body (JSON):**
    ```json
    // Example Success:
    {
      "mac_address": "AA:BB:CC:DD:EE:FF",
      "firmware_id": 5, // The ID of the firmware attempted
      "status": "success",
      "error_message": null
    }

    // Example Failure:
    {
      "mac_address": "AA:BB:CC:DD:EE:FF",
      "firmware_id": 5,
      "status": "failed",
      "error_message": "Checksum validation failed" // Or "Download failed: timeout", etc.
    }
    ```
*   **Response (HTTP 200):**
    ```json
    { "status": "logged" }
    ```

### 7. Rollback (Built-in)

The ESP32's OTA mechanism has a built-in rollback feature. If the newly flashed firmware crashes repeatedly or fails basic integrity checks during boot, the bootloader can automatically revert to the previously working firmware partition. Your backend logging (`/update-status`) helps diagnose which devices failed post-update.

## Authentication from ESP32

Based on the provided FastAPI code (`ota.py`, `firmware.py`, `device.py`):

*   The endpoints `/api/ota/check-update`, `/api/firmware/{firmware_id}/download`, `/api/ota/update-status`, and `/api/devices/register` **do not currently require authentication**. They don't use `Depends(get_current_admin)` or similar security dependencies.

**This means:**

*   Your ESP32 device **does not need** to obtain or send any authentication tokens (like JWT Bearer tokens) to perform the OTA check, download, or status reporting functions *as the backend is currently written*.

**Security Considerations:**

*   **Simplicity:** This lack of authentication simplifies the device-side code significantly.
*   **Risk:** Anyone who knows a valid device MAC address and the server URL *could* potentially:
    *   Check for updates for that device.
    *   Download firmware files (if they know a valid `firmware_id`).
    *   Post fake update statuses for a device.
    *   Register a device if the MAC isn't already taken.
*   **Mitigation:**
    *   Using **HTTPS** is essential to prevent eavesdropping on MAC addresses and firmware URLs.
    *   The backend *does* check if the MAC exists before proceeding with checks/status updates.
    *   Firmware downloads require knowing a valid `firmware_id`, which isn't easily guessable.
    *   Network-level security (firewalls, VPNs) can restrict access to the server.

**If Authentication Becomes Necessary:**

If you decide to secure these OTA endpoints in the future, you would typically:

1.  **Backend Changes:**
    *   Add authentication dependencies (`Depends(...)`) to the relevant FastAPI route functions in `ota.py` and potentially `firmware.py`.
    *   Decide on an authentication method suitable for devices (e.g., unique API keys per device, device-specific tokens, mutual TLS). JWT tokens used for human admins might not be ideal for unattended devices due to expiration and refresh complexity.
2.  **ESP32 Changes:**
    *   Implement the chosen authentication mechanism on the device.
    *   If using API keys or tokens, add the appropriate HTTP header (e.g., `Authorization: Bearer <token>` or `X-API-Key: <key>`) to all requests made to the newly secured endpoints.
    *   Securely store credentials on the device (e.g., in NVS, potentially encrypted).

**Conclusion on Authentication:** For the *current* backend implementation, no specific authentication steps are needed for the ESP32 OTA process itself. Focus on implementing the core workflow using HTTPS and robust error handling/validation.

## ESP-IDF / Arduino Implementation Notes

*   **HTTPS:** Strongly prefer `esp_https_ota` or `HTTPClient` with a root CA certificate for server verification.
*   **Error Handling:** Implement retries for network errors. Handle non-2xx HTTP status codes gracefully.
*   **JSON:** Ensure your JSON parsing handles potential errors or unexpected response structures.
*   **Checksum:** Use a reliable SHA-256 implementation.
*   **Memory:** Be mindful of RAM usage, especially when handling HTTP requests/responses and JSON parsing. Stream parsing might be necessary for large JSONs (though the OTA check response is small). The firmware download itself is typically streamed to flash by the OTA libraries.
*   **NVS:** Store the server URL, current firmware version, and potentially WiFi credentials in Non-Volatile Storage.
*   **Logging:** Implement local logging on the ESP32 (e.g., via UART) to aid debugging.