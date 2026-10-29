from pydantic import BaseModel, Field
from typing import Optional
from datetime import datetime
from ..models.batch import BatchUpdateState

class BatchBase(BaseModel):
    name: str = Field(..., description="Name of the batch")
    description: Optional[str] = Field(None, description="Batch description")
    check_update_time: datetime = Field(..., description="Time for update checks")
    auto_update: bool = Field(False, description="Whether to automatically update devices")

class BatchCreate(BatchBase):
    pass

class BatchUpdate(BaseModel):
    name: Optional[str] = None
    description: Optional[str] = None
    check_update_time: Optional[datetime] = None
    auto_update: Optional[bool] = None

class BatchInDB(BatchBase):
    id: int
    update_state: BatchUpdateState
    last_update_check: Optional[datetime]
    last_update_trigger: Optional[datetime]
    devices_total: int
    devices_updated: int
    devices_failed: int
    created_at: datetime
    updated_at: datetime

    class Config:
        from_attributes = True

class BatchResponse(BatchInDB):
    pass

class BatchFirmwareAssign(BaseModel):
    firmware_id: int
    is_active: bool = True

class BatchUpdateTrigger(BaseModel):
    force: bool = Field(False, description="Force update even if no new firmware")
