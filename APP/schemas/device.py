from pydantic import BaseModel, Field
from typing import Optional
from datetime import datetime

class DeviceBase(BaseModel):
    mac_address: str = Field(..., description="Device MAC address")
    current_firmware_version: str = Field(..., description="Current firmware version")
    batch_id: Optional[int] = Field(None, description="Batch ID if assigned")
    status: str = Field("active", description="Device status")

class DeviceCreate(DeviceBase):
    pass

class DeviceUpdate(BaseModel):
    batch_id: Optional[int] = None
    status: Optional[str] = None

class DeviceInDB(DeviceBase):
    id: int
    status: str
    last_check_time: datetime
    last_update_time: datetime
    created_at: datetime
    updated_at: datetime

    class Config:
        from_attributes = True

class DeviceResponse(DeviceInDB):
    pass