"""add tables from supplied ER diagram

Revision ID: 002
Revises: e429538b831a

The supplied diagram contains two case-only naming collisions:
``devices`` is used by both firmware and smart-home records, and
``Schedules``/``schedules`` are separate in the diagram. They are merged into
the existing ``devices`` and ``schedules`` tables because the supported local
engines cannot safely keep both case-only variants.
"""
from typing import Sequence, Union

from alembic import op
import sqlalchemy as sa


revision: str = "002"
down_revision: Union[str, None] = "e429538b831a"
branch_labels: Union[str, Sequence[str], None] = None
depends_on: Union[str, Sequence[str], None] = None


def upgrade() -> None:
    """Add the non-firmware tables and compatible diagram columns."""
    op.add_column("devices", sa.Column("device_id", sa.String(14), nullable=True))
    op.add_column("devices", sa.Column("device_topic", sa.String(180), nullable=True))
    op.add_column("devices", sa.Column("device_name", sa.String(255), nullable=True))
    op.add_column("devices", sa.Column("device_type", sa.String(100), nullable=True))
    op.add_column("devices", sa.Column("load_names", sa.String(255), nullable=True))
    op.add_column("devices", sa.Column("room_id", sa.Integer(), nullable=True))
    op.add_column("devices", sa.Column("user_id", sa.Integer(), nullable=True))

    op.create_table(
        "users",
        sa.Column("id", sa.Integer(), primary_key=True),
        sa.Column("username", sa.String(50), nullable=False),
        sa.Column("email", sa.String(100), nullable=False),
        sa.Column("password_hash", sa.String(255), nullable=False),
        sa.Column("is_verified", sa.Boolean(), nullable=False, server_default=sa.false()),
        sa.Column("created_at", sa.DateTime(), nullable=True),
        sa.Column("updated_at", sa.DateTime(), nullable=True),
    )
    op.create_table(
        "user_data",
        sa.Column("id", sa.Integer(), primary_key=True),
        sa.Column("username", sa.String(80), nullable=False),
        sa.Column("email", sa.String(120), nullable=False),
        sa.Column("password", sa.String(250), nullable=False),
        sa.Column("user_role", sa.String(200), nullable=True),
        sa.Column("remember_me", sa.Boolean(), nullable=True),
        sa.Column("verified", sa.Boolean(), nullable=True),
    )
    op.create_table(
        "homes",
        sa.Column("id", sa.Integer(), primary_key=True),
        sa.Column("home_name", sa.String(255), nullable=False),
        sa.Column("created_by", sa.Integer(), sa.ForeignKey("users.id"), nullable=True),
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
        "scenes",
        sa.Column("id", sa.Integer(), primary_key=True),
        sa.Column("scene", sa.JSON(), nullable=False),
        sa.Column("created_at", sa.DateTime(), nullable=True),
        sa.Column("updated_at", sa.DateTime(), nullable=True),
        sa.Column("user", sa.Integer(), sa.ForeignKey("users.id"), nullable=True),
    )
    op.create_table(
        "schedules",
        sa.Column("id", sa.Integer(), primary_key=True),
        sa.Column("scene_id", sa.Integer(), sa.ForeignKey("scenes.id"), nullable=True),
        sa.Column("schedule_name", sa.String(255), nullable=True),
        sa.Column("home_id", sa.Integer(), sa.ForeignKey("homes.id"), nullable=True),
        sa.Column("repeat", sa.Boolean(), nullable=True),
        sa.Column("days", sa.String(100), nullable=True),
        sa.Column("time", sa.DateTime(), nullable=True),
        sa.Column("pause", sa.Boolean(), nullable=True),
        sa.Column("created_at", sa.DateTime(), nullable=True),
        sa.Column("updated_at", sa.DateTime(), nullable=True),
        sa.Column("user_id", sa.Integer(), sa.ForeignKey("users.id"), nullable=True),
        sa.Column("topic", sa.String(100), nullable=True),
        sa.Column("switch_type", sa.Integer(), nullable=True),
        sa.Column("switch_no", sa.Integer(), nullable=True),
        sa.Column("status", sa.Boolean(), nullable=True),
        sa.Column("value", sa.Integer(), nullable=True),
        sa.Column("aps_job_id", sa.String(255), nullable=True),
    )
    op.create_table(
        "scene_schedules",
        sa.Column("id", sa.Integer(), primary_key=True),
        sa.Column("scene_id", sa.Integer(), sa.ForeignKey("scenes.id"), nullable=False),
        sa.Column("scene_data", sa.JSON(), nullable=False),
        sa.Column("time", sa.DateTime(), nullable=False),
        sa.Column("repeat", sa.Boolean(), nullable=True),
        sa.Column("days", sa.String(100), nullable=True),
        sa.Column("aps_job_id", sa.String(255), nullable=True),
    )
    op.create_table(
        "white_listed_devices",
        sa.Column("id", sa.Integer(), primary_key=True),
        sa.Column("device_id", sa.String(13), nullable=False),
        sa.Column("device_topic_publish", sa.String(13), nullable=True),
        sa.Column("device_topic_subscribe", sa.String(255), nullable=True),
        sa.Column("device_status", sa.Boolean(), nullable=True),
        sa.Column("user_id", sa.Integer(), sa.ForeignKey("users.id"), nullable=True),
    )
    op.create_table(
        "favorite_devices",
        sa.Column("id", sa.BigInteger(), primary_key=True),
        sa.Column("user_id", sa.Integer(), sa.ForeignKey("users.id"), nullable=False),
        sa.Column("device_id", sa.String(14), nullable=False),
        sa.Column("added_at", sa.DateTime(), nullable=True),
    )
    op.create_table(
        "frequent_devices",
        sa.Column("id", sa.BigInteger(), primary_key=True),
        sa.Column("user_id", sa.Integer(), sa.ForeignKey("users.id"), nullable=False),
        sa.Column("device_id", sa.String(14), nullable=False),
        sa.Column("usage_count", sa.Integer(), nullable=False),
        sa.Column("last_used_at", sa.DateTime(), nullable=True),
    )
    op.create_table(
        "home_members",
        sa.Column("id", sa.Integer(), primary_key=True),
        sa.Column("home_id", sa.Integer(), sa.ForeignKey("homes.id"), nullable=False),
        sa.Column("user_id", sa.Integer(), sa.ForeignKey("users.id"), nullable=False),
        sa.Column("role", sa.String(50), nullable=True),
        sa.Column("added_at", sa.DateTime(), nullable=True),
    )
    op.create_table(
        "active_home",
        sa.Column("id", sa.Integer(), primary_key=True),
        sa.Column("user_id", sa.Integer(), sa.ForeignKey("users.id"), nullable=False),
        sa.Column("home_id", sa.Integer(), sa.ForeignKey("homes.id"), nullable=False),
        sa.Column("set_at", sa.DateTime(), nullable=True),
    )
    op.create_table(
        "guest_access",
        sa.Column("id", sa.Integer(), primary_key=True),
        sa.Column("home_id", sa.Integer(), sa.ForeignKey("homes.id"), nullable=False),
        sa.Column("user_id", sa.Integer(), sa.ForeignKey("users.id"), nullable=False),
        sa.Column("room_id", sa.Integer(), sa.ForeignKey("rooms.id"), nullable=True),
        sa.Column("added_at", sa.DateTime(), nullable=True),
    )
    op.create_table(
        "notifications",
        sa.Column("id", sa.Integer(), primary_key=True),
        sa.Column("user_id", sa.Integer(), sa.ForeignKey("users.id"), nullable=False),
        sa.Column("message", sa.String(255), nullable=False),
        sa.Column("home_id", sa.Integer(), sa.ForeignKey("homes.id"), nullable=True),
        sa.Column("device_id", sa.String(14), nullable=True),
        sa.Column("room_id", sa.Integer(), sa.ForeignKey("rooms.id"), nullable=True),
        sa.Column("created_at", sa.DateTime(), nullable=True),
    )
    op.create_table(
        "otp_table",
        sa.Column("id", sa.Integer(), primary_key=True),
        sa.Column("user_id", sa.Integer(), sa.ForeignKey("users.id"), nullable=False),
        sa.Column("otp", sa.String(6), nullable=False),
        sa.Column("otp_expiry", sa.DateTime(), nullable=False),
        sa.Column("created_at", sa.DateTime(), nullable=True),
    )
    op.create_table(
        "user_auth_alexa",
        sa.Column("id", sa.Integer(), primary_key=True),
        sa.Column("email", sa.String(120), nullable=False),
        sa.Column("auth_code", sa.String(250), nullable=True),
        sa.Column("code_created_at", sa.DateTime(), nullable=True),
        sa.Column("access_token", sa.String(250), nullable=True),
        sa.Column("token_created_at", sa.DateTime(), nullable=True),
        sa.Column("refresh_token", sa.String(250), nullable=True),
        sa.Column("refresh_token_created_at", sa.DateTime(), nullable=True),
        sa.Column("user_id", sa.Integer(), sa.ForeignKey("users.id"), nullable=True),
    )
    op.create_table(
        "alexa_auth_requests",
        sa.Column("id", sa.Integer(), primary_key=True),
        sa.Column("state", sa.Text(), nullable=False),
        sa.Column("auth_code", sa.String(255), nullable=True),
        sa.Column("user_id_token", sa.Text(), nullable=True),
        sa.Column("client_id", sa.String(255), nullable=True),
        sa.Column("redirect_uri", sa.Text(), nullable=True),
        sa.Column("scope", sa.String(255), nullable=True),
        sa.Column("created_at", sa.DateTime(), nullable=True),
        sa.Column("expires_at", sa.DateTime(), nullable=True),
    )
    op.create_table(
        "user_refresh_token",
        sa.Column("id", sa.Integer(), primary_key=True),
        sa.Column("user_identifier", sa.String(255), nullable=False),
        sa.Column("alexa_refresh_token", sa.String(255), nullable=True),
        sa.Column("firebase_refresh_token", sa.Text(), nullable=True),
        sa.Column("created_at", sa.DateTime(), nullable=True),
        sa.Column("updated_at", sa.DateTime(), nullable=True),
    )
    op.create_table(
        "sessions",
        sa.Column("id", sa.Integer(), primary_key=True),
        sa.Column("session_id", sa.String(255), nullable=False),
        sa.Column("data", sa.LargeBinary(), nullable=True),
        sa.Column("expiry", sa.DateTime(), nullable=True),
    )
    op.create_table(
        "apscheduler_jobs",
        sa.Column("id", sa.String(191), primary_key=True),
        sa.Column("next_run_time", sa.Float(), nullable=True),
        sa.Column("job_state", sa.LargeBinary(), nullable=False),
    )


def downgrade() -> None:
    """Remove only the tables and columns introduced by this revision."""
    for table in (
        "apscheduler_jobs", "sessions", "user_refresh_token", "alexa_auth_requests",
        "user_auth_alexa", "otp_table", "notifications", "guest_access", "active_home",
        "home_members", "frequent_devices", "favorite_devices", "white_listed_devices",
        "scene_schedules", "schedules", "scenes", "rooms", "homes", "user_data", "users",
    ):
        op.drop_table(table)

    for column in (
        "user_id", "room_id", "load_names", "device_type", "device_name",
        "device_topic", "device_id",
    ):
        op.drop_column("devices", column)
