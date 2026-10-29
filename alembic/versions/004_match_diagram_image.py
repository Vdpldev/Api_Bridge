"""match the local schema to the supplied ER diagram image

Revision ID: 004
Revises: 003

The image contains the firmware-update tables and the home/device tables only.
Migration bookkeeping in ``alembic_version`` is retained intentionally.
"""
from typing import Sequence, Union

from alembic import op
import sqlalchemy as sa


revision: str = "004"
down_revision: Union[str, None] = "003"
branch_labels: Union[str, Sequence[str], None] = None
depends_on: Union[str, Sequence[str], None] = None


IMAGE_TABLES = {
    "device_update_logs",
    "firmware",
    "batch_firmware",
    "batches",
    "devices",
    "frequent_devices",
    "notifications",
    "home_members",
    "favorite_devices",
    "homes",
    "rooms",
    "active_home",
    "guest_access",
}


def upgrade() -> None:
    """Remove tables not shown in the supplied diagram image."""
    # Recreate these tables without references to the users table, which is
    # not present in the image and is removed below.
    for table in (
        "frequent_devices", "favorite_devices", "home_members",
        "active_home", "guest_access", "notifications", "rooms", "homes",
    ):
        op.drop_table(table)

    for table in (
        "user_auth_alexa",
        "user_refresh_token",
        "alexa_auth_requests",
        "otp_table",
        "sessions",
        "apscheduler_jobs",
        "scene_schedules",
        "schedules",
        "scenes",
        "admin_logs",
        "admin_users",
        "users",
    ):
        op.drop_table(table)

    op.create_table(
        "homes",
        sa.Column("id", sa.Integer(), primary_key=True),
        sa.Column("home_name", sa.String(255), nullable=False),
        sa.Column("created_by", sa.Integer(), nullable=True),
        sa.Column("created_at", sa.DateTime(), nullable=True),
        sa.Column("updated_at", sa.DateTime(), nullable=True),
    )
    op.create_table(
        "rooms",
        sa.Column("id", sa.Integer(), primary_key=True),
        sa.Column("room_name", sa.String(255), nullable=False),
        sa.Column("home_id", sa.Integer(), sa.ForeignKey("homes.id"), nullable=False),
    )

    op.create_table(
        "frequent_devices",
        sa.Column("id", sa.BigInteger(), primary_key=True),
        sa.Column("user_id", sa.Integer(), nullable=False),
        sa.Column("device_id", sa.String(14), nullable=False),
        sa.Column("usage_count", sa.Integer(), nullable=False),
        sa.Column("last_used_at", sa.DateTime(), nullable=True),
    )
    op.create_table(
        "favorite_devices",
        sa.Column("id", sa.BigInteger(), primary_key=True),
        sa.Column("user_id", sa.Integer(), nullable=False),
        sa.Column("device_id", sa.String(14), nullable=False),
        sa.Column("added_at", sa.DateTime(), nullable=True),
    )
    op.create_table(
        "home_members",
        sa.Column("id", sa.Integer(), primary_key=True),
        sa.Column("home_id", sa.Integer(), sa.ForeignKey("homes.id"), nullable=False),
        sa.Column("user_id", sa.Integer(), nullable=False),
        sa.Column("role", sa.String(50), nullable=True),
        sa.Column("added_at", sa.DateTime(), nullable=True),
    )
    op.create_table(
        "active_home",
        sa.Column("id", sa.Integer(), primary_key=True),
        sa.Column("user_id", sa.Integer(), nullable=False),
        sa.Column("home_id", sa.Integer(), sa.ForeignKey("homes.id"), nullable=False),
        sa.Column("set_at", sa.DateTime(), nullable=True),
    )
    op.create_table(
        "guest_access",
        sa.Column("id", sa.Integer(), primary_key=True),
        sa.Column("home_id", sa.Integer(), sa.ForeignKey("homes.id"), nullable=False),
        sa.Column("user_id", sa.Integer(), nullable=False),
        sa.Column("room_id", sa.Integer(), sa.ForeignKey("rooms.id"), nullable=True),
        sa.Column("added_at", sa.DateTime(), nullable=True),
    )
    op.create_table(
        "notifications",
        sa.Column("id", sa.Integer(), primary_key=True),
        sa.Column("user_id", sa.Integer(), nullable=False),
        sa.Column("message", sa.String(255), nullable=False),
        sa.Column("home_id", sa.Integer(), sa.ForeignKey("homes.id"), nullable=True),
        sa.Column("device_id", sa.String(14), nullable=True),
        sa.Column("room_id", sa.Integer(), sa.ForeignKey("rooms.id"), nullable=True),
        sa.Column("created_at", sa.DateTime(), nullable=True),
    )


def downgrade() -> None:
    """Restore removed tables by rerunning the previous diagram migration."""
    raise NotImplementedError(
        "This cleanup migration is intentionally forward-only; restore the local database from backup."
    )
