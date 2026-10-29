from pydantic import BaseModel, Field
from typing import Optional
from datetime import datetime
from enum import Enum

class LogLevel(str, Enum):
    DEBUG = "DEBUG"  
    INFO = "INFO"    
    WARNING = "WARNING"
    ERROR = "ERROR"

class UpdateLogBase(BaseModel):
    device_id: int
    firmware_id: int
    status: str
    error_message: Optional[str] = None
    level: LogLevel = LogLevel.INFO

class UpdateLogCreate(UpdateLogBase):
    pass

class UpdateLogInDB(UpdateLogBase):
    id: int
    created_at: datetime
    updated_at: datetime

    class Config:
        from_attributes = True

class AdminLogBase(BaseModel):
    admin_id: int
    action_type: str = Field(..., description="Type of action performed")
    entity_type: str = Field(..., description="Type of entity affected")
    entity_id: int = Field(..., description="ID of the affected entity")
    details: Optional[str] = Field(None, description="Additional details about the action")
    level: LogLevel = LogLevel.INFO

class AdminLogCreate(AdminLogBase):
    pass

class AdminLogInDB(AdminLogBase):
    id: int
    created_at: datetime

    class Config:
        from_attributes = True

class LogFilter(BaseModel):
    start_date: Optional[datetime] = None
    end_date: Optional[datetime] = None
    level: Optional[LogLevel] = None
    entity_type: Optional[str] = None
    entity_id: Optional[int] = None
    action_type: Optional[str] = None