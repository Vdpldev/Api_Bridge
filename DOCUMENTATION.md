# Firmware Update System Documentation

## Table of Contents

1. [System Overview](#system-overview)
2. [Architecture](#architecture)
3. [Data Models](#data-models)
4. [API Documentation](#api-documentation)
5. [Authentication and Authorization](#authentication-and-authorization)
6. [Batch Update Process](#batch-update-process)
7. [OTA Update Process](#ota-update-process)
8. [Logging System](#logging-system)
9. [Installation and Setup](#installation-and-setup)
10. [Configuration](#configuration)

## System Overview

The Firmware Update System is a comprehensive solution for managing and distributing firmware updates to IoT devices. It provides a centralized platform for administrators to manage firmware versions, organize devices into batches, and control the update process. The system supports Over-The-Air (OTA) updates, allowing devices to check for and download firmware updates automatically.

### Key Features

- **Device Management**: Register, organize, and monitor devices
- **Firmware Management**: Upload, version, and distribute firmware files
- **Batch Management**: Group devices for controlled update rollouts
- **OTA Updates**: Enable devices to check for and download updates
- **Admin Interface**: Manage users, permissions, and system settings
- **Logging and Monitoring**: Track update status and system activities

## Architecture

The system is built using a modern, scalable architecture with the following components:

### Backend

- **FastAPI**: High-performance web framework for building APIs
- **SQLAlchemy**: SQL toolkit and Object-Relational Mapping (ORM) library
- **Alembic**: Database migration tool
- **MySQL**: Relational database for data storage

### Authentication

- **JWT (JSON Web Tokens)**: For secure authentication
- **OAuth2**: For authorization flow
- **Passlib with Bcrypt**: For password hashing

### File Storage

- Local file system storage for firmware binary files

## Data Models

The system uses several data models to represent entities and their relationships. Below is a detailed explanation of each model, its purpose, and how it relates to other models.

### AdminUser

The `AdminUser` model represents administrators who manage the system.

```python
class AdminUser(Base):
    __tablename__ = 'admin_users'

    id = Column(Integer, primary_key=True, index=True)
    username = Column(String(255), unique=True, nullable=False)
    email = Column(String(255), unique=True, nullable=False)
    hashed_password = Column(String(255), nullable=False)
    role = Column(Enum('admin', 'manager', 'viewer', name='user_roles'), nullable=False)
    is_active = Column(Boolean, default=True)
    last_login = Column(DateTime, nullable=True)
    created_at = Column(DateTime, default=datetime.now(tz=timezone.utc))
    updated_at = Column(DateTime, default=datetime.now(tz=timezone.utc), onupdate=datetime.now(tz=timezone.utc))
```

**Purpose**: 
- Manages authentication and authorization for system administrators
- Controls access to different parts of the system based on roles
- Tracks administrative actions through logs

**Roles**:
- **admin**: Full access to all system features
- **manager**: Can manage devices, batches, and firmware, but cannot manage other admins
- **viewer**: Read-only access to system data

### Device

The `Device` model represents IoT devices that receive firmware updates.

```python
class Device(Base):
    __tablename__ = 'devices'

    id = Column(Integer, primary_key=True, index=True)
    mac_address = Column(String(50), unique=True, nullable=False)
    current_firmware_version = Column(String(50), nullable=False)
    batch_id = Column(Integer, ForeignKey('batches.id'))
    status = Column(String(50), nullable=False)
    last_check_time = Column(DateTime, default=datetime.now(tz=timezone.utc))
    last_update_time = Column(DateTime, default=datetime.now(tz=timezone.utc))
    created_at = Column(DateTime, default=datetime.now(tz=timezone.utc))
    updated_at = Column(DateTime, default=datetime.now(tz=timezone.utc))
```

**Purpose**:
- Tracks individual devices in the system
- Stores device identification (MAC address)
- Maintains current firmware version
- Associates devices with batches for update management
- Tracks device status and update history

**Status Values**:
- **active**: Device is operational and can receive updates
- **maintenance**: Device is under maintenance and should not receive updates
- **decommissioned**: Device is no longer in use

### Batch

The `Batch` model represents groups of devices that can be updated together.

```python
class Batch(Base):
    __tablename__ = 'batches'

    id = Column(Integer, primary_key=True, index=True)
    name = Column(String(255), nullable=False)
    description = Column(Text, nullable=True)
    check_update_time = Column(DateTime, nullable=False)
    auto_update = Column(Boolean, default=False)
    update_state = Column(SQLAlchemyEnum(BatchUpdateState), default=BatchUpdateState.IDLE)
    last_update_check = Column(DateTime, nullable=True)
    last_update_trigger = Column(DateTime, nullable=True)
    devices_total = Column(Integer, default=0)
    devices_updated = Column(Integer, default=0)
    devices_failed = Column(Integer, default=0)
    created_at = Column(DateTime, default=datetime.now(tz=timezone.utc))
    updated_at = Column(DateTime, default=datetime.now(tz=timezone.utc), onupdate=datetime.now(tz=timezone.utc))
```

**Purpose**:
- Groups devices for controlled update rollouts
- Schedules automatic update checks
- Tracks update progress and status
- Provides statistics on update success/failure rates

**BatchUpdateState Enum**:
```python
class BatchUpdateState(str, Enum):
    IDLE = "IDLE"
    UPDATING = "UPDATING"
    COMPLETED = "COMPLETED"
    FAILED = "FAILED"
```

### Firmware

The `Firmware` model represents firmware versions that can be deployed to devices.

```python
class Firmware(Base):
    __tablename__ = 'firmware'

    id = Column(Integer, primary_key=True, index=True)
    version = Column(String(50), unique=True, nullable=False)
    update_name = Column(String(255), nullable=False)
    release_notes = Column(Text, nullable=True)
    file_path = Column(String(255), nullable=False)
    file_size = Column(Integer, nullable=False)
    checksum = Column(String(255), nullable=False)
    is_beta = Column(Boolean, default=False)
    created_at = Column(DateTime, default=datetime.now(tz=timezone.utc))
    updated_at = Column(DateTime, default=datetime.now(tz=timezone.utc), onupdate=datetime.now(tz=timezone.utc))
```

**Purpose**:
- Stores metadata about firmware versions
- Tracks firmware binary files
- Provides version control for updates
- Includes checksums for file integrity verification

### BatchFirmware

The `BatchFirmware` model represents the association between batches and firmware versions.

```python
class BatchFirmware(Base):
    __tablename__ = 'batch_firmware'

    batch_id = Column(Integer, ForeignKey('batches.id'), primary_key=True)
    firmware_id = Column(Integer, ForeignKey('firmware.id'), primary_key=True)
    is_active = Column(Boolean, default=True)
    created_at = Column(DateTime, default=datetime.now(tz=timezone.utc))
```

**Purpose**:
- Creates a many-to-many relationship between batches and firmware
- Allows multiple firmware versions to be associated with a batch
- Tracks which firmware version is currently active for a batch

### DeviceUpdateLog

The `DeviceUpdateLog` model tracks firmware update attempts for devices.

```python
class DeviceUpdateLog(Base):
    __tablename__ = 'device_update_logs'

    id = Column(Integer, primary_key=True, index=True)
    device_id = Column(Integer, ForeignKey('devices.id'), nullable=False)
    firmware_id = Column(Integer, ForeignKey('firmware.id'), nullable=False)
    status = Column(String(50), nullable=False)
    error_message = Column(Text, nullable=True)
    level = Column(Enum(LogLevel), nullable=False, default=LogLevel.INFO)
    created_at = Column(DateTime, default=datetime.now(tz=timezone.utc))
```

**Purpose**:
- Records update attempts and results
- Tracks success/failure status
- Stores error messages for troubleshooting
- Provides audit trail for device updates

**Status Values**:
- **success**: Update completed successfully
- **failed**: Update failed
- **in_progress**: Update is currently in progress

### AdminLog

The `AdminLog` model tracks administrative actions in the system.

```python
class AdminLog(Base):
    __tablename__ = 'admin_logs'

    id = Column(Integer, primary_key=True, index=True)
    admin_id = Column(Integer, ForeignKey('admin_users.id'), nullable=False)
    action_type = Column(String(50), nullable=False)
    entity_type = Column(String(50), nullable=False)
    entity_id = Column(Integer, nullable=False)
    details = Column(Text, nullable=True)
    level = Column(Enum(LogLevel), nullable=False, default=LogLevel.INFO)
    created_at = Column(DateTime, default=datetime.now(tz=timezone.utc))
```

**Purpose**:
- Records administrative actions
- Provides audit trail for system changes
- Tracks who made changes and when
- Stores details about the changes made

**LogLevel Enum**:
```python
class LogLevel(str, Enum):
    DEBUG = "DEBUG"
    INFO = "INFO"
    WARNING = "WARNING"
    ERROR = "ERROR"
```

## API Documentation

The system provides a comprehensive REST API for interacting with all aspects of the firmware update system. Below is detailed documentation for each endpoint.

### Authentication API

#### Login for Access Token

- **Endpoint**: `POST /api/auth/token`
- **Description**: Authenticates an admin user and returns a JWT token
- **Request Body**:
  ```json
  {
    "username": "string",
    "password": "string"
  }
  ```
- **Response**:
  ```json
  {
    "access_token": "string",
    "token_type": "bearer"
  }
  ```
- **Status Codes**:
  - 200: Successful authentication
  - 401: Invalid credentials

#### Admin User Management

##### Create Admin User

- **Endpoint**: `POST /api/auth/admins`
- **Description**: Creates a new admin user
- **Authorization**: Requires admin role
- **Request Body**:
  ```json
  {
    "username": "string",
    "email": "string",
    "password": "string",
    "role": "admin|manager|viewer"
  }
  ```
- **Response**: Admin user object
- **Status Codes**:
  - 200: Admin created successfully
  - 400: Username already exists
  - 401: Unauthorized

##### List Admin Users

- **Endpoint**: `GET /api/auth/admins`
- **Description**: Lists all admin users
- **Authorization**: Requires admin role
- **Query Parameters**:
  - `skip`: Number of records to skip (default: 0)
  - `limit`: Maximum number of records to return (default: 100)
- **Response**: Array of admin user objects
- **Status Codes**:
  - 200: Success
  - 401: Unauthorized

##### Get Admin User

- **Endpoint**: `GET /api/auth/admins/{admin_id}`
- **Description**: Gets a specific admin user by ID
- **Authorization**: Requires admin role
- **Response**: Admin user object
- **Status Codes**:
  - 200: Success
  - 401: Unauthorized
  - 404: Admin not found

##### Update Admin User

- **Endpoint**: `PUT /api/auth/admins/{admin_id}`
- **Description**: Updates an admin user
- **Authorization**: Requires admin role
- **Request Body**:
  ```json
  {
    "email": "string",
    "role": "admin|manager|viewer",
    "is_active": true,
    "password": "string"
  }
  ```
- **Response**: Updated admin user object
- **Status Codes**:
  - 200: Success
  - 401: Unauthorized
  - 404: Admin not found

##### Delete Admin User

- **Endpoint**: `DELETE /api/auth/admins/{admin_id}`
- **Description**: Deletes an admin user
- **Authorization**: Requires admin role
- **Response**: Success message
- **Status Codes**:
  - 200: Success
  - 400: Cannot delete yourself
  - 401: Unauthorized
  - 404: Admin not found

### Device API

#### Register Device

- **Endpoint**: `POST /api/devices/register`
- **Description**: Registers a new device in the system
- **Request Body**:
  ```json
  {
    "mac_address": "string",
    "current_firmware_version": "string",
    "batch_id": "integer|null",
    "status": "string"
  }
  ```
- **Response**: Device object
- **Status Codes**:
  - 201: Device created successfully
  - 400: Device with this MAC address already exists

#### List Devices

- **Endpoint**: `GET /api/devices/`
- **Description**: Lists all devices
- **Authorization**: Requires admin, manager, or viewer role
- **Query Parameters**:
  - `skip`: Number of records to skip (default: 0)
  - `limit`: Maximum number of records to return (default: 100)
  - `batch_id`: Filter by batch ID (optional)
- **Response**: Array of device objects
- **Status Codes**:
  - 200: Success
  - 401: Unauthorized

#### Get Device

- **Endpoint**: `GET /api/devices/{device_id}`
- **Description**: Gets a specific device by ID
- **Authorization**: Requires admin, manager, or viewer role
- **Response**: Device object
- **Status Codes**:
  - 200: Success
  - 401: Unauthorized
  - 404: Device not found

#### Update Device

- **Endpoint**: `PUT /api/devices/{device_id}`
- **Description**: Updates a device
- **Authorization**: Requires admin or manager role
- **Request Body**:
  ```json
  {
    "batch_id": "integer|null",
    "status": "string"
  }
  ```
- **Response**: Updated device object
- **Status Codes**:
  - 200: Success
  - 401: Unauthorized
  - 404: Device not found

#### Delete Device

- **Endpoint**: `DELETE /api/devices/{device_id}`
- **Description**: Deletes a device
- **Authorization**: Requires admin or manager role
- **Status Codes**:
  - 204: Success, no content
  - 401: Unauthorized
  - 404: Device not found

### Firmware API

#### Upload Firmware

- **Endpoint**: `POST /api/firmware/`
- **Description**: Uploads a new firmware version
- **Authorization**: Requires admin or manager role
- **Request Body**: Multipart form data
  - `version`: Firmware version string
  - `update_name`: Name of the update
  - `release_notes`: Release notes (optional)
  - `is_beta`: Whether this is a beta release (default: false)
  - `file`: Binary firmware file
- **Response**: Firmware object
- **Status Codes**:
  - 201: Firmware created successfully
  - 400: Firmware version already exists
  - 401: Unauthorized

#### List Firmware

- **Endpoint**: `GET /api/firmware/`
- **Description**: Lists all firmware versions
- **Authorization**: Requires admin, manager, or viewer role
- **Query Parameters**:
  - `skip`: Number of records to skip (default: 0)
  - `limit`: Maximum number of records to return (default: 100)
  - `include_beta`: Whether to include beta versions (default: false)
- **Response**: Array of firmware objects
- **Status Codes**:
  - 200: Success
  - 401: Unauthorized

#### Get Firmware

- **Endpoint**: `GET /api/firmware/{firmware_id}`
- **Description**: Gets a specific firmware by ID
- **Authorization**: Requires admin, manager, or viewer role
- **Response**: Firmware object
- **Status Codes**:
  - 200: Success
  - 401: Unauthorized
  - 404: Firmware not found

#### Update Firmware

- **Endpoint**: `PUT /api/firmware/{firmware_id}`
- **Description**: Updates firmware metadata
- **Authorization**: Requires admin or manager role
- **Request Body**:
  ```json
  {
    "update_name": "string",
    "release_notes": "string",
    "is_beta": "boolean"
  }
  ```
- **Response**: Updated firmware object
- **Status Codes**:
  - 200: Success
  - 401: Unauthorized
  - 404: Firmware not found

#### Download Firmware

- **Endpoint**: `GET /api/firmware/{firmware_id}/download`
- **Description**: Downloads the firmware binary file
- **Response**: Binary file
- **Status Codes**:
  - 200: Success
  - 404: Firmware not found or file not found

#### Delete Firmware

- **Endpoint**: `DELETE /api/firmware/{firmware_id}`
- **Description**: Deletes a firmware version
- **Authorization**: Requires admin role
- **Status Codes**:
  - 204: Success, no content
  - 401: Unauthorized
  - 404: Firmware not found

### Batch API

#### Create Batch

- **Endpoint**: `POST /api/batches/`
- **Description**: Creates a new batch
- **Authorization**: Requires admin or manager role
- **Request Body**:
  ```json
  {
    "name": "string",
    "description": "string",
    "check_update_time": "datetime",
    "auto_update": "boolean"
  }
  ```
- **Response**: Batch object
- **Status Codes**:
  - 201: Batch created successfully
  - 401: Unauthorized

#### List Batches

- **Endpoint**: `GET /api/batches/`
- **Description**: Lists all batches
- **Authorization**: Requires admin, manager, or viewer role
- **Query Parameters**:
  - `skip`: Number of records to skip (default: 0)
  - `limit`: Maximum number of records to return (default: 100)
- **Response**: Array of batch objects
- **Status Codes**:
  - 200: Success
  - 401: Unauthorized

#### Get Batch

- **Endpoint**: `GET /api/batches/{batch_id}`
- **Description**: Gets a specific batch by ID
- **Authorization**: Requires admin, manager, or viewer role
- **Response**: Batch object
- **Status Codes**:
  - 200: Success
  - 401: Unauthorized
  - 404: Batch not found

#### Update Batch

- **Endpoint**: `PUT /api/batches/{batch_id}`
- **Description**: Updates a batch
- **Authorization**: Requires admin or manager role
- **Request Body**:
  ```json
  {
    "name": "string",
    "description": "string",
    "check_update_time": "datetime",
    "auto_update": "boolean"
  }
  ```
- **Response**: Updated batch object
- **Status Codes**:
  - 200: Success
  - 401: Unauthorized
  - 404: Batch not found

#### Delete Batch

- **Endpoint**: `DELETE /api/batches/{batch_id}`
- **Description**: Deletes a batch
- **Authorization**: Requires admin role
- **Status Codes**:
  - 204: Success, no content
  - 401: Unauthorized
  - 404: Batch not found

#### Assign Firmware to Batch

- **Endpoint**: `POST /api/batches/{batch_id}/firmware`
- **Description**: Assigns a firmware version to a batch
- **Authorization**: Requires admin or manager role
- **Request Body**:
  ```json
  {
    "firmware_id": "integer",
    "is_active": "boolean"
  }
  ```
- **Response**: Success message
- **Status Codes**:
  - 200: Success
  - 400: Could not assign firmware
  - 401: Unauthorized
  - 404: Batch not found

#### Get Batch Devices

- **Endpoint**: `GET /api/batches/{batch_id}/devices`
- **Description**: Gets all devices in a batch
- **Authorization**: Requires admin, manager, or viewer role
- **Query Parameters**:
  - `skip`: Number of records to skip (default: 0)
  - `limit`: Maximum number of records to return (default: 100)
- **Response**: Array of device objects
- **Status Codes**:
  - 200: Success
  - 401: Unauthorized
  - 404: Batch not found

#### Trigger Batch Update

- **Endpoint**: `POST /api/batches/{batch_id}/trigger-update`
- **Description**: Triggers an update for all devices in a batch
- **Authorization**: Requires admin or manager role
- **Request Body**:
  ```json
  {
    "force": "boolean"
  }
  ```
- **Response**: Update status
- **Status Codes**:
  - 200: Success
  - 400: Batch is already updating
  - 401: Unauthorized
  - 404: Batch not found

### OTA API

#### Check for Update

- **Endpoint**: `GET /api/ota/check-update`
- **Description**: Checks if an update is available for a device
- **Query Parameters**:
  - `mac_address`: Device MAC address
  - `current_version`: Current firmware version
- **Response**:
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
- **Status Codes**:
  - 200: Success
  - 404: Device not found

#### Report Update Status

- **Endpoint**: `POST /api/ota/update-status`
- **Description**: Reports the status of a firmware update
- **Request Body**:
  ```json
  {
    "mac_address": "string",
    "firmware_id": "integer",
    "status": "string",
    "error_message": "string|null"
  }
  ```
- **Response**: Success message
- **Status Codes**:
  - 200: Success
  - 404: Device or firmware not found

### Logs API

#### Get Device Logs

- **Endpoint**: `GET /api/logs/device-logs`
- **Description**: Gets logs for device updates
- **Authorization**: Requires admin, manager, or viewer role
- **Query Parameters**:
  - `device_id`: Filter by device ID (optional)
  - `start_date`: Filter by start date (optional)
  - `end_date`: Filter by end date (optional)
  - `level`: Filter by log level (optional)
  - `skip`: Number of records to skip (default: 0)
  - `limit`: Maximum number of records to return (default: 100)
- **Response**: Array of device update log objects
- **Status Codes**:
  - 200: Success
  - 401: Unauthorized

#### Get Admin Logs

- **Endpoint**: `GET /api/logs/admin-logs`
- **Description**: Gets logs for administrative actions
- **Authorization**: Requires admin role
- **Query Parameters**:
  - `start_date`: Filter by start date (optional)
  - `end_date`: Filter by end date (optional)
  - `level`: Filter by log level (optional)
  - `entity_type`: Filter by entity type (optional)
  - `entity_id`: Filter by entity ID (optional)
  - `action_type`: Filter by action type (optional)
  - `skip`: Number of records to skip (default: 0)
  - `limit`: Maximum number of records to return (default: 100)
- **Response**: Array of admin log objects
- **Status Codes**:
  - 200: Success
  - 401: Unauthorized

## Authentication and Authorization

The system uses JWT (JSON Web Tokens) for authentication and role-based access control for authorization.

### Authentication Flow

1. Admin users authenticate by sending their username and password to the `/api/auth/token` endpoint
2. The system verifies the credentials and returns a JWT token
3. Subsequent API requests include the token in the Authorization header
4. The system validates the token and identifies the user for each request

### Authorization Roles

The system supports three roles with different permission levels:

1. **admin**: Full access to all system features
   - Can manage other admin users
   - Can delete batches and firmware
   - Has access to all logs and statistics

2. **manager**: Can manage devices, batches, and firmware
   - Cannot manage admin users
   - Cannot delete batches or firmware
   - Has access to device logs and statistics

3. **viewer**: Read-only access to system data
   - Can view devices, batches, and firmware
   - Can view device logs
   - Cannot make any changes to the system

## Batch Update Process

The batch update process allows for controlled rollout of firmware updates to groups of devices.

### Batch Update States

1. **IDLE**: The batch is not currently updating
2. **UPDATING**: The batch is in the process of updating devices
3. **COMPLETED**: The batch update has completed successfully
4. **FAILED**: The batch update has failed

### Automatic Updates

Batches can be configured for automatic updates:

1. Set `auto_update` to `true` for a batch
2. Set `check_update_time` to schedule when updates should be checked
3. The system will automatically check for updates at the specified time
4. If a new firmware version is available, devices in the batch will be updated

### Manual Updates

Administrators can also trigger updates manually:

1. Assign a firmware version to a batch
2. Use the `/api/batches/{batch_id}/trigger-update` endpoint to start the update
3. Set `force` to `true` to update all devices even if they already have the latest version

## OTA Update Process

The Over-The-Air (OTA) update process allows devices to check for and download firmware updates.

### Device Update Flow

1. **Check for Update**:
   - Device sends its MAC address and current firmware version to `/api/ota/check-update`
   - System checks if a newer firmware version is available for the device's batch
   - If available, system returns firmware metadata and download URL

2. **Download Firmware**:
   - Device downloads the firmware binary from the provided URL
   - Device verifies the firmware checksum

3. **Apply Update**:
   - Device applies the firmware update
   - Device reports the update status to `/api/ota/update-status`

4. **Update Status**:
   - System records the update status in the device update logs
   - System updates the device's current firmware version if successful
   - System updates batch statistics (devices_updated, devices_failed)

## Logging System

The system maintains comprehensive logs for both device updates and administrative actions.

### Device Update Logs

Device update logs track the status of firmware updates for each device:

- **Success**: Update completed successfully
- **Failed**: Update failed with an error message
- **In Progress**: Update is currently in progress

### Admin Logs

Admin logs track administrative actions in the system:

- User management (create, update, delete)
- Batch management (create, update, delete)
- Firmware management (upload, update, delete)
- Batch update triggers

### Log Levels

Both log types support different severity levels:

- **DEBUG**: Detailed information for debugging
- **INFO**: General information about system operation
- **WARNING**: Potential issues that don't prevent operation
- **ERROR**: Errors that prevent successful operation

## Installation and Setup

### Prerequisites

- Python 3.8 or higher
- MySQL database
- Virtual environment (recommended)

### Installation Steps

1. Clone the repository
2. Create and activate a virtual environment
3. Install dependencies:
   ```
   pip install -r requirements.txt
   ```
4. Create a `.env` file with configuration settings (see Configuration section)
5. Initialize the database:
   ```
   alembic upgrade head
   ```
6. Start the application:
   ```
   uvicorn main:app --reload
   ```

## Configuration

The system is configured using environment variables, which can be set in a `.env` file.

### Required Configuration

```
# Database settings
DATABASE_URL=mysql+aiomysql://user:password@host:port/database

# Firmware storage settings
FIRMWARE_STORAGE_PATH=./firmware_files

# Security settings
SECRET_KEY=your-secret-key

# Application settings
DEBUG=True
```

### Optional Configuration

```
# API key for external integrations
API_KEY=your-api-key

# Application name
APP_NAME=Firmware Update Service
```

### Database Migration

The system uses Alembic for database migrations:

- Create a new migration:
  ```
  alembic revision --autogenerate -m "description"
  ```
- Apply migrations:
  ```
  alembic upgrade head
  ```
- Rollback migrations:
  ```
  alembic downgrade -1
  ```
