from pydantic import BaseModel, Field
from typing import Optional
from datetime import datetime

class FirmwareBase(BaseModel):
    version: str = Field(..., description="Firmware version number")
    update_name: str = Field(..., description="Name of the update")
    release_notes: Optional[str] = Field(None, description="Release notes")
    is_beta: bool = Field(False, description="Whether this is a beta release")

class FirmwareCreate(FirmwareBase):
    pass

class FirmwareUpdate(BaseModel):
    update_name: Optional[str] = None
    release_notes: Optional[str] = None
    is_beta: Optional[bool] = None

class FirmwareInDB(FirmwareBase):
    id: int
    file_path: str
    file_size: int
    checksum: str
    created_at: datetime
    updated_at: datetime

    class Config:
        from_attributes = True

class FirmwareResponse(FirmwareInDB):
    pass