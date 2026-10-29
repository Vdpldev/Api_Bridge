from sqlalchemy.ext.asyncio import AsyncSession
from sqlalchemy import select
from typing import List, Optional
from datetime import datetime, timezone
from APP.models.admin import AdminUser
from APP.schemas.admin import AdminCreate, AdminUpdate
from APP.core.security import get_password_hash, verify_password
import logging

logger = logging.getLogger(__name__)

logging.basicConfig(
    filename='api.log',
    level=logging.INFO,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s'
)


async def create_admin(db: AsyncSession, admin: AdminCreate) -> AdminUser:
    existing_email = await get_admin_by_email(db, admin.email)
    if  existing_email is not None:
        logger.warning(f"check the return of email validator {admin.email}")
        raise ValueError("Email already registered")
    
    existing_username = await get_admin_by_username(db, admin.username)
    if  existing_username is not None:
        raise ValueError("Username already registered")
    
    hashed_password = get_password_hash(admin.password)
    db_admin = AdminUser(
        username=admin.username,
        email=admin.email,
        role=admin.role,
        hashed_password=hashed_password
    )

    db.add(db_admin)
    await db.flush()
    await db.refresh(db_admin)
    logger.info(f"Admin user {admin.username} created successfully with ID {db_admin.id}")
    return db_admin

async def get_admin_by_id(db: AsyncSession, admin_id: int) -> Optional[AdminUser]:
    result = await db.execute(select(AdminUser).filter(AdminUser.id == admin_id))
    return result.scalar_one_or_none()

async def get_admin_by_email(db: AsyncSession, email: str) -> Optional[AdminUser]:
    result = await db.execute(select(AdminUser).filter(AdminUser.email == email))
    return result.scalar_one_or_none()


async def get_admin_by_username(db: AsyncSession, username: str) -> Optional[AdminUser]:
    result = await db.execute(select(AdminUser).filter(AdminUser.username == username))
    return result.scalar_one_or_none()

async def get_admins(
    db: AsyncSession,
    skip: int = 0,
    limit: int = 100
) -> List[AdminUser]:
    query = select(AdminUser).offset(skip).limit(limit)
    result = await db.execute(query)
    return result.scalars().all()

async def update_admin(
    db: AsyncSession,
    admin: AdminUser,
    admin_update: AdminUpdate
) -> AdminUser:
    update_data = admin_update.dict(exclude_unset=True)
    
    if "password" in update_data:
        update_data["hashed_password"] = get_password_hash(update_data.pop("password"))
    
    for field, value in update_data.items():
        setattr(admin, field, value)
    
    db.add(admin)
    await db.flush()
    await db.refresh(admin)
    return admin

async def authenticate_admin(
    db: AsyncSession,
    username: str,
    password: str
) -> Optional[AdminUser]:
    admin = await get_admin_by_username(db, username)
    if not admin:
        return None
    if not verify_password(password, admin.hashed_password):
        return None
    
    # Update last login time
    admin.last_login = datetime.now(tz=timezone.utc)
    # await db.commit()
    db.add(admin)
    await db.flush()
    
    return admin

async def delete_admin(db: AsyncSession, admin: AdminUser) -> None:
    username = admin.username # Get username for logging before deletion
    await db.delete(admin)
    # await db.commit()  # <--- REMOVE THIS
    await db.flush()     # <--- ADD THIS
    logger.info(f"Admin user {username} deleted successfully.")