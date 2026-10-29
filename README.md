# Firmware Update Service

An asynchronous FastAPI service for managing IoT devices and distributing firmware updates. The service provides device registration, firmware storage, OTA update support, batch rollouts, administrator authentication, and update logging.

> **Python version:** This project is tested with **Python 3.12**. Use Python 3.12 for both the virtual environment and application runtime. Other Python versions may not work with the pinned dependencies.

## Features

- Device registration and device lifecycle management
- Firmware upload, versioning, download, and metadata management
- OTA update checks for devices
- Firmware assignment to device batches
- Manual and scheduled batch update processing
- Batch update status and progress tracking
- JWT-based administrator authentication
- Role-based administrator access (`admin`, `manager`, and `viewer`)
- Password hashing with bcrypt
- Device update logs, administrator logs, and statistics
- Automatic API documentation through FastAPI
- Async database access with SQLAlchemy
- Database schema migrations with Alembic
- Local firmware binary storage
- Health-check endpoint for monitoring

## Technology Stack

- Python 3.12
- FastAPI
- Uvicorn
- SQLAlchemy 2 with async sessions
- MySQL with `aiomysql`
- Alembic
- Pydantic Settings
- JWT authentication
- Passlib and bcrypt

## Project Structure

```text
.
+-- APP/
|   +-- api/endpoints/       API route modules
|   +-- core/                Application settings and security
|   +-- crud/                Database operations
|   +-- db/                  Database engine and sessions
|   +-- middleware/          Authentication and request logging
|   +-- models/              SQLAlchemy models
|   +-- schemas/             Pydantic request and response schemas
|   +-- services/            Background batch update service
+-- alembic/                 Database migration scripts
+-- firmware_files/          Local firmware binary storage
+-- tests/                   Automated tests
+-- main.py                  FastAPI application entry point
+-- requirements.txt         Pinned Python dependencies
+-- API_DOCUMENTATION.md     Endpoint reference
+-- DOCUMENTATION.md         Architecture and workflow documentation
```

## Prerequisites

Install the following before starting:

1. Windows 10/11 (the commands below use PowerShell).
2. Python **3.12.x**.
3. Git.
4. A running MySQL server and a database for the application.

Verify Python and Git:

```powershell
py -3.12 --version
git --version
```

The Python command must report version 3.12. If `py -3.12` is not available, install Python 3.12 and enable the Python launcher during installation.

## Installation

### 1. Clone the repository and select the server branch

```powershell
git clone --branch Server https://github.com/Vdpldev/Api_Bridge.git
cd Api_Bridge
```

If the repository is already cloned:

```powershell
git checkout Server
git pull origin Server
```

### 2. Create a Python 3.12 virtual environment

```powershell
py -3.12 -m venv venv312
```

Activate it in PowerShell:

```powershell
.\venv312\Scripts\Activate.ps1
```

If PowerShell blocks activation, allow scripts for the current user, then activate again:

```powershell
Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope CurrentUser
.\venv312\Scripts\Activate.ps1
```

Confirm that the environment is active and uses Python 3.12:

```powershell
python --version
where.exe python
```

The `where.exe python` output should point into `venv312\Scripts`.

### 3. Upgrade packaging tools and install dependencies

```powershell
python -m pip install --upgrade pip setuptools wheel
python -m pip install -r requirements.txt
```

Always use `python -m pip` after activating the environment so packages are installed into the Python 3.12 environment.

## Configuration

Create a `.env` file in the project root. Do not commit this file because it contains secrets and environment-specific settings.

Example:

```dotenv
DATABASE_URL=mysql://your_mysql_user:your_mysql_password@localhost:3306/firmware_db
FIRMWARE_STORAGE_PATH=./firmware_files
SECRET_KEY=replace-with-a-long-random-secret
DEBUG=true

# Optional settings
APP_NAME=Firmware Update Service
BATCH_CHECK_INTERVAL=300
DEVICE_UPDATE_TIMEOUT=3600
LOG_LEVEL=INFO
LOG_FILE=app.log
API_KEY=
RATE_LIMIT_ENABLED=false
RATE_LIMIT_PER_MINUTE=60
```

### Configuration variables

| Variable | Required | Description |
| --- | --- | --- |
| `DATABASE_URL` | Yes | Database URL. MySQL is the supported deployment database. |
| `FIRMWARE_STORAGE_PATH` | Yes | Directory where uploaded firmware files are stored. |
| `SECRET_KEY` | Yes | Secret used to sign JWT access tokens. Use a unique random value. |
| `DEBUG` | Yes | Enables debug behavior and SQL echoing when `true`. Use `false` in production. |
| `APP_NAME` | No | Application name. |
| `BATCH_CHECK_INTERVAL` | No | Background batch check interval in seconds. Default: `300`. |
| `DEVICE_UPDATE_TIMEOUT` | No | Maximum device update timeout in seconds. Default: `3600`. |
| `LOG_LEVEL` | No | Logging level. Default: `INFO`. |
| `LOG_FILE` | No | Optional log file name. |
| `CORS_ORIGINS` | No | Allowed frontend origins. Configure this explicitly for production. |
| `API_KEY` | No | Optional API key setting. |

The application converts a standard MySQL URL to the async SQLAlchemy driver automatically:

```text
mysql://user:password@host:3306/database
```

becomes:

```text
mysql+aiomysql://user:password@host:3306/database
```

Create the MySQL database before running migrations:

```sql
CREATE DATABASE firmware_db CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;
```

## Database Setup

The repository includes Alembic migrations. Before running them, set the `sqlalchemy.url` value in `alembic.ini` to the same database used by `DATABASE_URL`. Use a sync MySQL URL for Alembic:

```ini
sqlalchemy.url = mysql+pymysql://your_mysql_user:your_mysql_password@localhost:3306/firmware_db
```

Do not commit real database credentials in `alembic.ini`.

Run all migrations:

```powershell
alembic upgrade head
```

The application also initializes its database models during startup. Migrations are still recommended because they provide a controlled, repeatable schema history.

## Run the Application

With `venv312` activated and `.env` configured:

```powershell
python -m uvicorn main:app --reload
```

The server is available at:

- API: <http://127.0.0.1:8000>
- Swagger UI: <http://127.0.0.1:8000/docs>
- ReDoc: <http://127.0.0.1:8000/redoc>
- Health check: <http://127.0.0.1:8000/health>

For a non-development run without auto-reload:

```powershell
python -m uvicorn main:app --host 0.0.0.0 --port 8000
```

When the application starts, it starts the background batch update service. On shutdown, that service is stopped gracefully.

## API Areas

All API routes are grouped under these prefixes:

| Area | Prefix | Purpose |
| --- | --- | --- |
| Authentication | `/api/auth` | Login and administrator management |
| Devices | `/api/devices` | Register and manage devices |
| Firmware | `/api/firmware` | Upload and manage firmware releases |
| OTA | `/api/ota` | Device update checks and update status |
| Batches | `/api/batches` | Group devices and control rollouts |
| Logs | `/api/logs` | Device logs, admin logs, and statistics |

Authentication flow:

1. Obtain a JWT token from `POST /api/auth/token`.
2. Send the token with protected requests:

   ```text
   Authorization: Bearer <access-token>
   ```

3. Use the Swagger UI's **Authorize** button to test protected endpoints interactively.

See [API_DOCUMENTATION.md](./API_DOCUMENTATION.md) for request bodies, response formats, and endpoint details.

## First Run Checklist

1. Confirm Python reports version 3.12.
2. Activate `venv312`.
3. Install `requirements.txt`.
4. Create the MySQL database.
5. Create `.env` with a valid `DATABASE_URL`, `FIRMWARE_STORAGE_PATH`, `SECRET_KEY`, and `DEBUG`.
6. Configure `sqlalchemy.url` in `alembic.ini` without committing credentials.
7. Run `alembic upgrade head`.
8. Start the server with Uvicorn.
9. Open `/docs` and call `/health`.
10. Create or use an administrator account, then obtain a JWT token.
11. Register a device and upload a firmware release before configuring a batch update.

## Run Tests

Activate the Python 3.12 virtual environment, then run:

```powershell
python -m pytest
```

The current tests focus on password handling and security behavior.

## Important Security and Production Notes

- Never commit `.env`, database passwords, JWT secrets, access tokens, or production logs.
- Use a long, unique `SECRET_KEY` in every deployment.
- Set `DEBUG=false` in production.
- Replace development CORS origins with the exact production frontend origins.
- Run behind a reverse proxy with HTTPS in production.
- Restrict firmware storage permissions and validate uploaded firmware before distribution.
- Use a dedicated MySQL user with only the permissions required by this service.
- Review the default/system administrator setup in `main.py` before deploying. Change any development credentials and move credentials to environment-based configuration.
- Configure backups for the MySQL database and firmware storage.
- Configure log rotation and monitor `/health`.
- Rate limiting is currently disabled by default and is marked for production implementation.

## Troubleshooting

### `ModuleNotFoundError`

Make sure the virtual environment is activated and reinstall dependencies:

```powershell
.\venv312\Scripts\Activate.ps1
python -m pip install -r requirements.txt
```

### Settings validation errors at startup

Check that `.env` exists in the project root and contains all required values:

- `DATABASE_URL`
- `FIRMWARE_STORAGE_PATH`
- `SECRET_KEY`
- `DEBUG`

### Database connection errors

Check that MySQL is running, the database exists, the credentials are correct, and the URL uses the expected host and port.

### Alembic connection errors

Check the `sqlalchemy.url` entry in `alembic.ini`. Alembic uses that value independently of the application's `.env` settings.

### PowerShell cannot activate the environment

Run:

```powershell
Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope CurrentUser
```

Then activate `venv312` again.

## Documentation

- [API documentation](./API_DOCUMENTATION.md)
- [Architecture and workflow documentation](./DOCUMENTATION.md)
- [GitHub repository](https://github.com/Vdpldev/Api_Bridge)
