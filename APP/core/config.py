from pathlib import Path
from typing import Optional
import os
from pydantic_settings import BaseSettings 

class Settings(BaseSettings):
    # Database settings
    DATABASE_URL: str
    
    # Firmware storage settings
    FIRMWARE_STORAGE_PATH: Path
    
    # Security settings
    SECRET_KEY: str
    API_KEY: Optional[str] = None
    
    # Application settings
    APP_NAME: str = "Firmware Update Service"
    DEBUG: bool
    
    # Batch update service settings
    BATCH_CHECK_INTERVAL: int = 300  # 5 minutes
    DEVICE_UPDATE_TIMEOUT: int = 3600  # 1 hour timeout for device updates
    
    # Logging settings
    LOG_LEVEL: str = "INFO"
    LOG_FILE: Optional[str] = "app.log"
    
    # CORS settings (for development)
    CORS_ORIGINS: list = ["http://localhost:3000", "http://localhost:8000"]
    
    # Rate limiting (TODO: Implement in production)
    RATE_LIMIT_ENABLED: bool = False
    RATE_LIMIT_PER_MINUTE: int = 60

    class Config:
        env_file = os.getenv("ENV_FILE", ".env")
        env_file_encoding = "utf-8"
        extra = "ignore"

# Create global settings instance
settings = Settings()
