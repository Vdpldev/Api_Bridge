from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware
from fastapi.responses import HTMLResponse
from APP.api.endpoints import device, firmware, ota, batch, auth, logs
from APP.middleware.auth import auth_middleware, logging_middleware
from starlette.middleware.base import BaseHTTPMiddleware
from APP.db.sessions import AsyncSessionLocal
from APP.crud import admin as admin_crud
from APP.schemas.admin import AdminCreate
import asyncio
from contextlib import asynccontextmanager
import logging

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger(__name__)

@asynccontextmanager
async def lifespan(app: FastAPI):
    """Manage application lifecycle - startup and shutdown"""
    logger.info("Starting up the application...")
    
    # Create system admin account on startup
    try:
        async with AsyncSessionLocal() as db:
            system_admin = await admin_crud.get_admin_by_username(db, "system")
            if not system_admin:
                logger.info("Creating system admin account...")
                system_admin = await admin_crud.create_admin(
                    db,
                    AdminCreate(
                        username="system",
                        email="cpavan783@gmail.com",
                        password="P@22@14c",  # TODO: Change in production - use environment variable
                        role="admin"
                    )
                )
                await db.commit()  # Ensure the admin is committed
            system_admin_id = system_admin.id
            logger.info(f"System admin ID: {system_admin_id}")
    except Exception as e:
        logger.error(f"Error during startup: {e}")
        raise

    # Start batch update service
    from APP.services.batch_update import BatchUpdateService
    
    batch_service = BatchUpdateService(system_admin_id)
    batch_task = asyncio.create_task(batch_service.run())
    
    logger.info("Batch update service started")
    
    yield  # Server is running and handling requests here
    
    # Cleanup on shutdown
    logger.info("Shutting down the application...")
    batch_service.stop()
    batch_task.cancel()
    try:
        await batch_task
    except asyncio.CancelledError:
        logger.info("Batch update service stopped")

app = FastAPI(
    title="Firmware Update Service",
    version="1.0.0",
    lifespan=lifespan
)

# Add CORS middleware for development
# TODO: Configure properly for production
app.add_middleware(
    CORSMiddleware,
    allow_origins=["null","http://localhost:3000", "http://localhost:8000", "http://192.168.1.102:5500"],  # Development origins
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

# Add custom middleware
app.add_middleware(BaseHTTPMiddleware, dispatch=logging_middleware)
app.add_middleware(BaseHTTPMiddleware, dispatch=auth_middleware)

# Include routers
app.include_router(auth.router, prefix="/api/auth", tags=["auth"])
app.include_router(device.router, prefix="/api/devices", tags=["devices"])
app.include_router(firmware.router, prefix="/api/firmware", tags=["firmware"])
app.include_router(ota.router, prefix="/api/ota", tags=["ota"])
app.include_router(batch.router, prefix="/api/batches", tags=["batches"])
app.include_router(logs.router, prefix="/api/logs", tags=["logs"])

# Health check endpoint
@app.get("/health")
async def health_check():
    """Health check endpoint for monitoring"""
    return {"status": "healthy", "service": "firmware-update-service"}

UI_HTML = r"""
<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8"><meta name="viewport" content="width=device-width, initial-scale=1">
<title>Firmware Control Room</title>
<style>
:root{--ink:#17212b;--muted:#64727e;--paper:#f6f8f5;--card:#fff;--line:#dce4df;--accent:#087f5b;--accent2:#f0a202;--danger:#c0392b;--shadow:0 14px 35px #173b2b12}*{box-sizing:border-box}body{margin:0;background:radial-gradient(circle at 90% 0,#d8f1e4 0,transparent 35%),var(--paper);color:var(--ink);font:15px Georgia,serif}button,input,select,textarea{font:inherit}button{border:0;border-radius:7px;padding:10px 14px;background:var(--accent);color:#fff;cursor:pointer;font-weight:bold}button:hover{filter:brightness(.92)}button.secondary{background:#e9efeb;color:var(--ink)}button.danger{background:var(--danger)}button.warn{background:var(--accent2);color:#241700}.hidden{display:none!important}.shell{max-width:1500px;margin:auto;padding:26px}.header{display:flex;justify-content:space-between;gap:20px;align-items:end;margin-bottom:22px}.eyebrow{color:var(--accent);text-transform:uppercase;letter-spacing:2px;font:12px Arial,sans-serif;font-weight:bold}.header h1{font-size:clamp(2rem,4vw,4.4rem);line-height:.92;margin:8px 0}.header p{color:var(--muted);margin:0;max-width:650px}.session{display:flex;gap:10px;align-items:center;font:13px Arial,sans-serif}.session strong{color:var(--accent)}.login{max-width:480px;margin:12vh auto;background:var(--card);padding:35px;box-shadow:var(--shadow);border-top:5px solid var(--accent)}.login h2{font-size:2rem;margin:0 0 20px}.nav{display:flex;gap:7px;flex-wrap:wrap;border-bottom:1px solid var(--line);margin-bottom:24px}.nav button{background:transparent;color:var(--muted);border-radius:0;padding:12px 16px}.nav button.active{color:var(--accent);border-bottom:3px solid var(--accent)}.section{display:none}.section.active{display:block;animation:rise .22s ease-out}@keyframes rise{from{opacity:0;transform:translateY(5px)}to{opacity:1;transform:none}}.grid{display:grid;grid-template-columns:repeat(12,1fr);gap:16px}.card{background:var(--card);border:1px solid var(--line);padding:20px;box-shadow:var(--shadow);min-width:0}.span-3{grid-column:span 3}.span-4{grid-column:span 4}.span-6{grid-column:span 6}.span-8{grid-column:span 8}.span-12{grid-column:1/-1}.card h2,.card h3{margin:0 0 15px}.metric{font:700 2.3rem Arial,sans-serif;color:var(--accent)}.label{color:var(--muted);font:12px Arial,sans-serif;text-transform:uppercase;letter-spacing:1px}.form-grid{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:12px}.full{grid-column:1/-1}label{display:block;color:var(--muted);font:12px Arial,sans-serif;font-weight:bold;margin:2px 0 6px}input,select,textarea{width:100%;border:1px solid var(--line);border-radius:6px;padding:10px;background:#fbfdfb;color:var(--ink)}textarea{min-height:82px;resize:vertical}.check{display:flex;gap:8px;align-items:center;margin-top:24px}.check input{width:auto}.actions{display:flex;gap:8px;flex-wrap:wrap;margin-top:14px}.status{min-height:20px;margin-top:10px;color:var(--accent);font:13px Arial,sans-serif}.status.error{color:var(--danger)}.table-wrap{overflow:auto}table{width:100%;border-collapse:collapse;font-family:Arial,sans-serif;font-size:13px}th,td{text-align:left;padding:11px;border-bottom:1px solid var(--line);vertical-align:top;white-space:nowrap}th{color:var(--muted);font-size:11px;text-transform:uppercase;letter-spacing:.6px}td.wrap{white-space:normal;min-width:180px}.pill{display:inline-block;border-radius:20px;padding:4px 8px;background:#e6f4ee;color:var(--accent);font:11px Arial,sans-serif;font-weight:bold}.pill.warn{background:#fff2d1;color:#946100}.pill.bad{background:#fde8e6;color:var(--danger)}.empty{color:var(--muted);padding:30px;text-align:center}.toolbar{display:flex;justify-content:space-between;gap:12px;align-items:center;margin-bottom:14px}.toolbar input{max-width:260px}.hint{color:var(--muted);font:13px Arial,sans-serif}.links{display:flex;gap:12px;margin-top:24px;font:13px Arial,sans-serif}.links a{color:var(--accent)}@media(max-width:900px){.span-3,.span-4,.span-6,.span-8{grid-column:span 6}}@media(max-width:600px){.shell{padding:16px}.header{display:block}.session{margin-top:18px}.span-3,.span-4,.span-6,.span-8{grid-column:1/-1}.form-grid{grid-template-columns:1fr}th,td{white-space:normal}.nav{overflow:auto;flex-wrap:nowrap}}
</style></head>
<body><div class="shell">
<header class="header"><div><div class="eyebrow">Operations console</div><h1>Firmware Control Room</h1><p>Manage releases, devices, rollout batches, and update health from one place.</p></div><div id="session" class="session hidden"><span>Signed in as <strong id="who"></strong></span><button class="secondary" id="logout">Sign out</button></div></header>
<main id="loginView" class="login"><h2>Welcome back</h2><p class="hint">Use an administrator account to access the control room.</p><form id="loginForm"><label>Username<input name="username" value="system" required autocomplete="username"></label><label>Password<input name="password" type="password" required autocomplete="current-password"></label><button>Sign in</button><div id="loginStatus" class="status"></div></form></main>
<main id="appView" class="hidden"><nav class="nav" id="nav"></nav><div id="content"></div><div class="links"><a href="/docs" target="_blank">Swagger API</a><a href="/redoc" target="_blank">ReDoc</a><a href="/health" target="_blank">Health</a></div></main>
</div>
<script>
const $=id=>document.getElementById(id);let token=localStorage.getItem('firmwareToken')||'';let role='viewer';
const esc=v=>String(v??'').replace(/[&<>"']/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));
const date=v=>v?new Date(v).toLocaleString():'-';const bytes=v=>{if(v==null)return'-';let n=v,u='B';if(n>1024){n/=1024;u='KB'}if(n>1024){n/=1024;u='MB'}return n.toFixed(n<10&&u!=='B'?1:0)+' '+u};
function decodeToken(token){const part=token.split('.')[1].replace(/-/g,'+').replace(/_/g,'/');const padded=part+'='.repeat((4-part.length%4)%4);return JSON.parse(atob(padded))}
function status(id,msg,error=false){const e=$(id);if(e){e.textContent=msg;e.className='status'+(error?' error':'')}}
async function api(path,options={}){if(token){options.headers={...(options.headers||{}),Authorization:'Bearer '+token}}const r=await fetch(path,options);const text=await r.text();let data={};try{data=text?JSON.parse(text):{}}catch{data={detail:text}}if(r.status===401){logout()}if(!r.ok)throw Error(data.detail||'Request failed ('+r.status+')');return data}
function json(method,body){return{method,headers:{'Content-Type':'application/json'},body:JSON.stringify(body)}}
function table(headers,rows){return '<div class="table-wrap"><table><thead><tr>'+headers.map(h=>'<th>'+h+'</th>').join('')+'</tr></thead><tbody>'+(rows.length?rows.join(''):'<tr><td colspan="'+headers.length+'" class="empty">No records found.</td></tr>')+'</tbody></table></div>'}
function initialTab(){const page=location.pathname.split('/').pop();return ['overview','devices','firmware','batches','logs','tools','explorer','admins'].includes(page)?page:'overview'}
function nav(){const items=[['overview','Overview'],['devices','Devices'],['firmware','Firmware'],['batches','Batches'],['logs','Logs'],['tools','Device tools'],['explorer','API explorer']];if(role==='admin')items.push(['admins','Team']);const active=initialTab();$('nav').innerHTML=items.map(x=>'<button class="'+(active===x[0]?'active':'')+'" data-tab="'+x[0]+'">'+x[1]+'</button>').join('');$('nav').onclick=e=>{if(e.target.dataset.tab){const page=e.target.dataset.tab;history.pushState({},'',page==='overview'?'/ui':'/ui/'+page);document.querySelectorAll('.nav button').forEach(b=>b.classList.remove('active'));e.target.classList.add('active');render(page)}}}
function card(title,body,cls='span-4'){return '<section class="card '+cls+'"><h2>'+title+'</h2>'+body+'</section>'}
async function render(tab){const c=$('content');c.innerHTML='<div class="card"><span class="hint">Loading '+tab+'...</span></div>';try{if(tab==='overview')await overview(c);if(tab==='devices')await devices(c);if(tab==='firmware')await firmware(c);if(tab==='batches')await batches(c);if(tab==='logs')await logs(c);if(tab==='admins')await admins(c);if(tab==='tools')tools(c);if(tab==='explorer')explorer(c)}catch(e){c.innerHTML='<section class="card"><div class="status error">'+esc(e.message)+'</div></section>'}}
async function overview(c){const [d,f,b]=await Promise.all([api('/api/devices/'),api('/api/firmware/'),api('/api/batches/')]);c.innerHTML='<div class="grid">'+card('Devices','<div class="metric">'+d.length+'</div><div class="label">registered</div>')+card('Releases','<div class="metric">'+f.length+'</div><div class="label">available</div>')+card('Batches','<div class="metric">'+b.length+'</div><div class="label">configured</div>')+card('Quick start','<p class="hint">Upload firmware, register devices, create a batch, then assign a release and trigger the rollout.</p><div class="actions"><button onclick="tabTo(\'firmware\')">Manage releases</button><button class="secondary" onclick="tabTo(\'batches\')">View rollouts</button></div>','span-12')+'</div>'}
function tabTo(t){document.querySelector('[data-tab="'+t+'"]')?.click()}
async function devices(c){const d=await api('/api/devices/');c.innerHTML='<div class="grid">'+card('Register device','<form id="deviceForm" class="form-grid"><label>MAC address<input name="mac_address" placeholder="AA:BB:CC:DD:EE:FF" required></label><label>Firmware version<input name="current_firmware_version" placeholder="1.0.0" required></label><label>Batch ID (optional)<input name="batch_id" type="number"></label><label>Status<select name="status"><option>active</option><option>maintenance</option><option>decommissioned</option></select></label><div class="full actions"><button>Register</button></div><div id="deviceStatus" class="status full"></div></form>','span-4')+card('Registered devices','<div class="toolbar"><span class="hint">'+d.length+' device(s)</span><input id="deviceSearch" placeholder="Filter devices"></div><div id="deviceTable"></div>','span-8')+'</div>';const draw=filter=>{$('deviceTable').innerHTML=table(['ID','Device','Firmware','Batch','Status','Last check','Actions'],d.filter(x=>(x.mac_address+' '+x.status).toLowerCase().includes(filter||'')).map(x=>'<tr><td>'+x.id+'</td><td>'+esc(x.mac_address)+'</td><td>'+esc(x.current_firmware_version)+'</td><td>'+(x.batch_id??'-')+'</td><td><span class="pill '+(x.status==='failed'?'bad':'')+'">'+esc(x.status)+'</span></td><td>'+date(x.last_check_time)+'</td><td><button class="danger" onclick="removeItem(\'/api/devices/'+x.id+'\',\'device\')">Delete</button></td></tr>'))};draw('');$('deviceSearch').oninput=e=>draw(e.target.value);$('deviceForm').onsubmit=async e=>{e.preventDefault();const f=new FormData(e.target);const body=Object.fromEntries(f);body.batch_id=body.batch_id?Number(body.batch_id):null;try{await api('/api/devices/register',json('POST',body));status('deviceStatus','Device registered.');await devices(c)}catch(x){status('deviceStatus',x.message,true)}}}
async function firmware(c){const f=await api('/api/firmware/');c.innerHTML='<div class="grid">'+card('Upload release','<form id="firmwareForm" class="form-grid"><label>Version<input name="version" placeholder="2.0.0" required></label><label>Release name<input name="update_name" placeholder="Summer release" required></label><label class="full">Release notes<textarea name="release_notes"></textarea></label><label class="check"><input name="is_beta" type="checkbox"> Beta release</label><label class="full">Firmware binary<input name="file" type="file" required></label><div class="full actions"><button>Upload release</button></div><div id="firmwareStatus" class="status full"></div></form>','span-4')+card('Release library','<div class="toolbar"><span class="hint">'+f.length+' release(s)</span><button class="secondary" onclick="render(\'firmware\')">Refresh</button></div><div id="firmwareTable"></div>','span-8')+'</div>';$('firmwareTable').innerHTML=table(['Version','Name','Size','Type','Created','Actions'],f.map(x=>'<tr><td><strong>'+esc(x.version)+'</strong></td><td class="wrap">'+esc(x.update_name)+'</td><td>'+bytes(x.file_size)+'</td><td><span class="pill '+(x.is_beta?'warn':'')+'">'+(x.is_beta?'Beta':'Stable')+'</span></td><td>'+date(x.created_at)+'</td><td><a href="/api/firmware/'+x.id+'/download" target="_blank"><button class="secondary">Download</button></a> <button class="danger" onclick="removeItem(\'/api/firmware/'+x.id+'\',\'release\')">Delete</button></td></tr>'));$('firmwareForm').onsubmit=async e=>{e.preventDefault();const data=new FormData(e.target);data.set('is_beta',data.has('is_beta')?'true':'false');try{await api('/api/firmware/',{method:'POST',body:data});status('firmwareStatus','Release uploaded.');await firmware(c)}catch(x){status('firmwareStatus',x.message,true)}}}
async function batches(c){const [b,f]=await Promise.all([api('/api/batches/'),api('/api/firmware/')]);c.innerHTML='<div class="grid">'+card('Create batch','<form id="batchForm" class="form-grid"><label>Name<input name="name" required></label><label>Update check time<input name="check_update_time" type="datetime-local" required></label><label class="full">Description<textarea name="description"></textarea></label><label class="check"><input name="auto_update" type="checkbox"> Automatic updates</label><div class="full actions"><button>Create batch</button></div><div id="batchStatus" class="status full"></div></form>','span-4')+card('Rollout batches','<div id="batchTable"></div>','span-8')+'</div>';$('batchTable').innerHTML=table(['Batch','State','Devices','Progress','Release','Actions'],b.map(x=>'<tr><td><strong>'+esc(x.name)+'</strong><br><span class="hint">'+esc(x.description||'')+'</span></td><td><span class="pill">'+x.update_state+'</span></td><td>'+x.devices_total+'</td><td>'+x.devices_updated+' updated / '+x.devices_failed+' failed</td><td><select id="fw-'+x.id+'">'+f.map(y=>'<option value="'+y.id+'">'+esc(y.version)+'</option>').join('')+'</select></td><td><button onclick="assign('+x.id+')">Assign</button> <button class="warn" onclick="trigger('+x.id+')">Trigger</button> <button class="danger" onclick="removeItem(\'/api/batches/'+x.id+'\',\'batch\')">Delete</button></td></tr>'));$('batchForm').onsubmit=async e=>{e.preventDefault();const body=Object.fromEntries(new FormData(e.target));body.auto_update=!!body.auto_update;body.check_update_time=new Date(body.check_update_time).toISOString();try{await api('/api/batches/',json('POST',body));status('batchStatus','Batch created.');await batches(c)}catch(x){status('batchStatus',x.message,true)}}}
async function assign(id){try{await api('/api/batches/'+id+'/assign-firmware',json('POST',{firmware_id:Number($('fw-'+id).value),is_active:true}));alert('Firmware assigned to batch.')}catch(e){alert(e.message)}}async function trigger(id){try{await api('/api/batches/'+id+'/trigger-update',json('POST',{force:false}));alert('Batch update triggered.')}catch(e){alert(e.message)}}
async function logs(c){const [d,s]=await Promise.all([api('/api/logs/device-logs?limit=100'),api('/api/logs/statistics')]);c.innerHTML='<div class="grid">'+card('Update activity',table(['Device','Firmware','Status','Level','Message','Time'],d.map(x=>'<tr><td>'+x.device_id+'</td><td>'+x.firmware_id+'</td><td><span class="pill">'+esc(x.status)+'</span></td><td>'+x.level+'</td><td class="wrap">'+esc(x.error_message||'-')+'</td><td>'+date(x.created_at)+'</td></tr>')),'span-8')+card('Statistics','<pre style="white-space:pre-wrap;font:13px Arial">'+esc(JSON.stringify(s,null,2))+'</pre>','span-4')+'</div>'}
async function admins(c){const a=await api('/api/auth/admins');c.innerHTML='<div class="grid">'+card('Create team member','<form id="adminForm" class="form-grid"><label>Username<input name="username" required></label><label>Email<input name="email" type="email" required></label><label>Password<input name="password" type="password" minlength="8" required></label><label>Role<select name="role"><option>viewer</option><option>manager</option><option>admin</option></select></label><div class="full actions"><button>Create member</button></div><div id="adminStatus" class="status full"></div></form>','span-4')+card('Team','<div id="adminTable"></div>','span-8')+'</div>';$('adminTable').innerHTML=table(['ID','Username','Email','Role','Active','Last login','Actions'],a.map(x=>'<tr><td>'+x.id+'</td><td>'+esc(x.username)+'</td><td>'+esc(x.email)+'</td><td><span class="pill">'+x.role+'</span></td><td>'+x.is_active+'</td><td>'+date(x.last_login)+'</td><td><button class="danger" onclick="removeItem(\'/api/auth/admins/'+x.id+'\',\'member\')">Delete</button></td></tr>'));$('adminForm').onsubmit=async e=>{e.preventDefault();try{await api('/api/auth/admins',json('POST',Object.fromEntries(new FormData(e.target))));status('adminStatus','Member created.');await admins(c)}catch(x){status('adminStatus',x.message,true)}}}
function tools(c){c.innerHTML='<div class="grid">'+card('Check for update','<form id="checkForm"><label>Device MAC<input name="mac_address" required></label><label>Current firmware version<input name="current_version" required></label><div class="actions"><button>Check device</button></div><div id="checkResult" class="status"></div></form>','span-6')+card('Report update status','<form id="statusForm" class="form-grid"><label>Device MAC<input name="mac_address" required></label><label>Firmware ID<input name="firmware_id" type="number" required></label><label>Status<select name="status"><option>downloading</option><option>installing</option><option>success</option><option>failed</option></select></label><label>Error message<input name="error_message"></label><div class="full actions"><button>Report status</button></div><div id="reportResult" class="status full"></div></form>','span-6')+card('Device information','<form id="infoForm"><label>Device MAC<input name="mac_address" required></label><div class="actions"><button>View device info</button></div><pre id="infoResult" class="hint" style="white-space:pre-wrap"></pre></form>','span-6')+'</div>';$('checkForm').onsubmit=async e=>{e.preventDefault();const q=new URLSearchParams(new FormData(e.target));try{status('checkResult',JSON.stringify(await api('/api/ota/checkupdate?'+q)))}catch(x){status('checkResult',x.message,true)}};$('statusForm').onsubmit=async e=>{e.preventDefault();const b=Object.fromEntries(new FormData(e.target));b.firmware_id=Number(b.firmware_id);try{status('reportResult',JSON.stringify(await api('/api/ota/update-status',json('POST',b))))}catch(x){status('reportResult',x.message,true)}};$('infoForm').onsubmit=async e=>{e.preventDefault();try{$('infoResult').textContent=JSON.stringify(await api('/api/ota/device-info/'+encodeURIComponent(new FormData(e.target).get('mac_address'))),null,2)}catch(x){$('infoResult').textContent=x.message}}}
function explorer(c){c.innerHTML='<div class="grid">'+card('Call any API endpoint','<form id="explorerForm" class="form-grid"><label>Method<select name="method"><option>GET</option><option>POST</option><option>PUT</option><option>DELETE</option></select></label><label>Path<input name="path" value="/api/firmware/" placeholder="/api/devices/1"></label><label class="full">JSON body<textarea name="body" placeholder="{ }"></textarea></label><div class="full actions"><button>Send request</button></div><div id="explorerStatus" class="status full"></div></form>','span-6')+card('Response','<pre id="explorerResult" class="hint" style="white-space:pre-wrap;max-height:480px;overflow:auto">Responses appear here.</pre>','span-6')+'</div>';$('explorerForm').onsubmit=async e=>{e.preventDefault();const f=new FormData(e.target),method=f.get('method'),path=f.get('path');if(!path.startsWith('/')){status('explorerStatus','Path must start with /',true);return}const options={method};if(method!=='GET'&&method!=='DELETE'){try{options.headers={'Content-Type':'application/json'};options.body=JSON.stringify(f.get('body')?JSON.parse(f.get('body')):{})}catch{status('explorerStatus','Body must be valid JSON.',true);return}}try{$('explorerResult').textContent=JSON.stringify(await api(path,options),null,2);status('explorerStatus','Request completed.')}catch(x){$('explorerResult').textContent=x.message;status('explorerStatus',x.message,true)}}}
async function removeItem(path,label){if(!confirm('Delete this '+label+'?'))return;try{await api(path,{method:'DELETE'});render(document.querySelector('.nav button.active').dataset.tab)}catch(e){alert(e.message)}}
function logout(){token='';localStorage.removeItem('firmwareToken');$('appView').classList.add('hidden');$('session').classList.add('hidden');$('loginView').classList.remove('hidden')}
$('loginForm').onsubmit=async e=>{e.preventDefault();try{const r=await api('/api/auth/token',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:new URLSearchParams(new FormData(e.target))});token=r.access_token;localStorage.setItem('firmwareToken',token);decodeToken(token);window.location.assign('/ui')}catch(x){status('loginStatus',x.message||'Incorrect username or password.',true)}};$('logout').onclick=logout;
if(token){try{const p=decodeToken(token);role=p.role||'viewer';$('who').textContent=p.sub||'user';$('loginView').classList.add('hidden');$('appView').classList.remove('hidden');$('session').classList.remove('hidden');nav();render(initialTab())}catch{logout()}}
window.onpopstate=()=>{if(token){nav();render(initialTab())}}
</script></body></html>
"""

# Root endpoint
@app.get("/", response_class=HTMLResponse)
async def root():
    """Serve a simple UI landing page for the service."""
    return UI_HTML

@app.get("/ui", response_class=HTMLResponse)
async def ui_page():
    """Alias route for the UI page."""
    return UI_HTML

@app.get("/ui/{page}", response_class=HTMLResponse)
async def ui_subpage(page: str):
    """Serve a bookmarkable management page from the single-page console."""
    return UI_HTML

# For local development
if __name__ == "__main__":
    import uvicorn
    # TODO: For production, use gunicorn with uvicorn workers
    # Example: gunicorn main:app -w 4 -k uvicorn.workers.UvicornWorker
    uvicorn.run(
        "main:app",
        host="0.0.0.0",
        port=8000,
        reload=True,  # Enable auto-reload for development
        log_level="info"
    )