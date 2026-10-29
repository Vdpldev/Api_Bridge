from fastapi import Request, HTTPException, status
from fastapi.security import OAuth2PasswordBearer
from APP.core.config import settings
from APP.db.sessions import get_session
import logging
from datetime import datetime, timezone
from jose import jwt, JWTError
from APP.core.security import SECRET_KEY, ALGORITHM
from APP.crud import admin as admin_crud
import time

# Configure logging
logger = logging.getLogger(__name__)

oauth2_scheme = OAuth2PasswordBearer(tokenUrl="/api/auth/token")

PUBLIC_PATHS = [
    "/api/auth/token",
    "/api/auth/admins",  # Allow creating first admin
    "/api/devices/register",  # Allow device registration
    "/api/ota/check-update",  # Allow devices to check updates
    "/api/ota/update-status",  # Allow devices to report status
    "/docs",
    "/redoc",
    "/openapi.json",
    "/health",
    "/",
]

# TODO: For production, implement:
# - Rate limiting per IP/user
# - Request ID tracking
# - Enhanced security headers
# - API key authentication for devices

async def auth_middleware(request: Request, call_next):
    """Authentication middleware for all requests"""
    # Check if path is public
    path = request.url.path
    if any(path.startswith(p) for p in PUBLIC_PATHS):
        return await call_next(request)
    
    # Extract token from header
    auth_header = request.headers.get("Authorization")
    if not auth_header or not auth_header.startswith("Bearer "):
        logger.warning(f"Missing or invalid auth header for {path}")
        raise HTTPException(
            status_code=status.HTTP_401_UNAUTHORIZED,
            detail="Not authenticated",
            headers={"WWW-Authenticate": "Bearer"},
        )
    
    try:
        token = auth_header.split(" ")[1]
        
        # Decode and validate token
        try:
            payload = jwt.decode(token, SECRET_KEY, algorithms=[ALGORITHM])
            username: str = payload.get("sub")
            
            if username is None:
                raise HTTPException(
                    status_code=status.HTTP_401_UNAUTHORIZED,
                    detail="Invalid token payload"
                )
                
        except jwt.ExpiredSignatureError:
            raise HTTPException(
                status_code=status.HTTP_401_UNAUTHORIZED,
                detail="Token has expired"
            )
        except JWTError as e:
            logger.error(f"JWT validation error: {e}")
            raise HTTPException(
                status_code=status.HTTP_401_UNAUTHORIZED,
                detail="Could not validate credentials"
            )
        
        # Store username in request state for later use
        request.state.username = username
        request.state.user_role = payload.get("role")
        
        # Process the request
        response = await call_next(request)
        return response
        
    except HTTPException:
        raise
    except Exception as e:
        logger.error(f"Unexpected error in auth middleware: {e}", exc_info=True)
        raise HTTPException(
            status_code=status.HTTP_500_INTERNAL_SERVER_ERROR,
            detail="Internal server error"
        )

async def logging_middleware(request: Request, call_next):
    """Logging middleware for request/response tracking"""
    # Generate request ID for tracking
    request_id = f"{int(time.time() * 1000)}-{hash(request.url.path) % 10000}"
    request.state.request_id = request_id
    
    start_time = datetime.now(tz=timezone.utc)
    
    # Log request
    logger.info(
        f"[{request_id}] Request: {request.method} {request.url.path} "
        f"Client: {request.client.host if request.client else 'Unknown'}"
    )
    
    # Add request ID to response headers
    response = await call_next(request)
    response.headers["X-Request-ID"] = request_id
    
    # Calculate process time
    process_time = (datetime.now(tz=timezone.utc) - start_time).total_seconds()
    
    # Log response
    logger.info(
        f"[{request_id}] Response: {response.status_code} "
        f"Process Time: {process_time:.3f}s"
    )
    
    # Add security headers for production
    # TODO: Adjust these based on your security requirements
    if not settings.DEBUG:
        response.headers["X-Content-Type-Options"] = "nosniff"
        response.headers["X-Frame-Options"] = "DENY"
        response.headers["X-XSS-Protection"] = "1; mode=block"
        response.headers["Strict-Transport-Security"] = "max-age=31536000; includeSubDomains"
    
    return response

# TODO: For production, consider adding:
# 1. API Key middleware for device authentication
# 2. Rate limiting middleware
# 3. CORS middleware with specific origins
# 4. Request size limiting
# 5. Timeout middleware for long-running requests