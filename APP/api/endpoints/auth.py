from datetime import timedelta, timezone
from fastapi import APIRouter, Depends, HTTPException, status
from fastapi.security import OAuth2PasswordBearer, OAuth2PasswordRequestForm
from sqlalchemy.ext.asyncio import AsyncSession
from jose import JWTError, jwt
from typing import List
from ...core.security import create_access_token, ACCESS_TOKEN_EXPIRE, SECRET_KEY, ALGORITHM
from ...schemas.auth import Token
from ...schemas.admin import AdminCreate, AdminUpdate, AdminResponse
from ...crud import admin as admin_crud
from ...db.sessions import get_session
import logging
import datetime


router = APIRouter()
oauth2_scheme = OAuth2PasswordBearer(tokenUrl="api/auth/token")

logging.basicConfig(
    filename='auth_routes.log',
    level=logging.INFO,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger(__name__)


@router.post("/token", response_model=Token)
async def login_for_access_token(
    form_data: OAuth2PasswordRequestForm = Depends(),
    db: AsyncSession = Depends(get_session)
):
    admin = await admin_crud.authenticate_admin(db, form_data.username, form_data.password)
    if not admin:
        raise HTTPException(
            status_code=status.HTTP_401_UNAUTHORIZED,
            detail="Incorrect username or password",
            headers={"WWW-Authenticate": "Bearer"},
        )
        
    access_token = create_access_token(
        data={"sub": admin.username,
              "exp": datetime.datetime.now(tz = timezone.utc) + timedelta(days=ACCESS_TOKEN_EXPIRE),
              "role" : admin.role},
    )
    return {"access_token": access_token, "token_type": "bearer"}

async def get_current_admin(
    token: str = Depends(oauth2_scheme),
    db: AsyncSession = Depends(get_session)
):
    try:
        payload = jwt.decode(token, SECRET_KEY, algorithms=[ALGORITHM])
        username: str = payload.get("sub")
        logger.info(f"Decoded JWT payload: {payload}")
        if username is None:
            logger.warning(f"JWT payload does not contain 'sub'")
            raise HTTPException(
                status_code=status.HTTP_401_UNAUTHORIZED,
                detail="Could not validate credentials"
            )
    except jwt.ExpiredSignatureError: # More specific error handling
        logger.warning(f"JWTError: Token has expired")
        raise HTTPException(
            status_code=status.HTTP_401_UNAUTHORIZED,
            detail="Token has expired"
        )
    except jwt.JWTError as e: # Catch generic JWT errors
        logger.warning(f"JWTError: Invalid token - {e}")
        raise HTTPException(
            status_code=status.HTTP_401_UNAUTHORIZED,
            detail="Could not validate credentials"
        )
    # Check if the token is expired
    admin = await admin_crud.get_admin_by_username(db, username)
    if admin is None:
        raise HTTPException(
            status_code=status.HTTP_401_UNAUTHORIZED,
            detail="User not found"
        )
    return admin

def check_admin_role(roles: List[str]):
    async def admin_role_checker(current_admin = Depends(get_current_admin)):
        if current_admin.role not in roles:
            raise HTTPException(
                status_code=status.HTTP_403_FORBIDDEN,
                detail=f"Operation requires one of these roles: {roles}"
            )
        return current_admin
    return admin_role_checker

# Admin user management endpoints
@router.post("/admins", response_model=AdminResponse)
async def create_admin_user(
    admin: AdminCreate,
    db: AsyncSession = Depends(get_session),
    # current_admin = Depends(check_admin_role(["admin"])),
):
    return await admin_crud.create_admin(db, admin)

@router.get("/admins", response_model=List[AdminResponse])
async def list_admins(
    skip: int = 0,
    limit: int = 100,
    db: AsyncSession = Depends(get_session),
    current_admin = Depends(check_admin_role(["admin"])),
):
    return await admin_crud.get_admins(db, skip=skip, limit=limit)

@router.get("/admins/{admin_id}", response_model=AdminResponse)
async def get_admin(
    admin_id: int,
    db: AsyncSession = Depends(get_session),
    # current_admin = Depends(check_admin_role(["admin"])),
):
    db_admin = await admin_crud.get_admin_by_id(db, admin_id)
    if not db_admin:
        raise HTTPException(status_code=404, detail="Admin not found")
    return db_admin

@router.put("/admins/{admin_id}", response_model=AdminResponse)
async def update_admin_user(
    admin_id: int,
    admin_update: AdminUpdate,
    db: AsyncSession = Depends(get_session),
    # current_admin = Depends(check_admin_role(["admin"])),
):
    db_admin = await admin_crud.get_admin_by_id(db, admin_id)
    if not db_admin:
        raise HTTPException(status_code=404, detail="Admin not found")
    return await admin_crud.update_admin(db, db_admin, admin_update)

@router.delete("/admins/{admin_id}")
async def delete_admin_user(
    admin_id: int,
    db: AsyncSession = Depends(get_session),
    current_admin = Depends(check_admin_role(["admin"])),
):
    db_admin = await admin_crud.get_admin_by_id(db, admin_id)
    if not db_admin:
        raise HTTPException(status_code=404, detail="Admin not found")
    if db_admin.id == current_admin.id:
        raise HTTPException(status_code=400, detail="Cannot delete yourself")
    await admin_crud.delete_admin(db, db_admin)
    return {"status": "success"}