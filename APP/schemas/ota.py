from pydantic import BaseModel
from typing import Optional, List

class UpdateStatusRequest(BaseModel):
    mac_address: str
    firmware_id: int
    status: str
    error_message: Optional[str] = None