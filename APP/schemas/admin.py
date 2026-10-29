from pydantic import BaseModel, EmailStr, Field
from typing import Optional
from datetime import datetime

class AdminBase(BaseModel):
    username: str = Field(..., min_length=3)
    email: EmailStr
    role: str = Field(..., pattern='^(admin|manager|viewer)$')

class AdminCreate(AdminBase):
    password: str = Field(..., min_length=8)

class AdminUpdate(BaseModel):
    email: Optional[EmailStr] = None
    role: Optional[str] = Field(None, pattern='^(admin|manager|viewer)$')
    is_active: Optional[bool] = None
    password: Optional[str] = Field(None, min_length=8)

class AdminInDB(AdminBase):
    id: int
    is_active: bool
    last_login: Optional[datetime]
    created_at: datetime
    updated_at: datetime

    class Config:
        from_attributes = True

class AdminResponse(AdminInDB):
    pass