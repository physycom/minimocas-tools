# launch with
# uvicorn simulation-ws:app --reload --port 9999

# and test with
# GET test
# curl --request GET http://localhost:9999
# POST test
# curl --header "Content-Type: application/json" --request POST --data '{"key": "value"}' http://localhost:9999/...

import sys
import os
import json
from jsonschema import validate, exceptions
from datetime import datetime
import pandas as pd
from fastapi import FastAPI, Request, HTTPException

##########################
#### log function ########
##########################
def log_print(*args, **kwargs):
  print('{} [sim-ws] '.format(datetime.now()), end='', flush=True)
  print(*args, **kwargs, flush=True)

#################
### libraries ###
#################
try:
  sys.path.append(os.path.join(os.environ['WORKSPACE'], 'minimocas', 'bin'))
  from pinimocas import simulation

  sys.path.append(os.path.join(os.environ['WORKSPACE'], 'minimocas', 'python', 'lib'))
  from conf_walker import conf_walker
  from conf_boat import conf_boat

except Exception as e:
  raise Exception('library loading error : {}'.format(e)) from e

try:
  schema_walker_file = os.path.join(os.environ['WORKSPACE'], 'minimocas', 'python', 'web', 'schemas', 'schema_walker_request.json')
  with open(schema_walker_file) as sin:
    schema_walker = json.load(sin)

  schema_boat_file = os.path.join(os.environ['WORKSPACE'], 'minimocas', 'python', 'web', 'schemas', 'schema_boat_request.json')
  with open(schema_boat_file) as sin:
    schema_boat = json.load(sin)

except Exception as e:
  raise Exception('schema file loading error : {}'.format(e)) from e

#################
#### fastAPI ####
#################
app = FastAPI()

@app.get("/")
async def root():
  return {"message": "scr simulation ws"}

@app.post("/sim_walker")
async def sim_walker_post(body: dict, request: Request):
  client_ip = request.client.host
  log_print('Request from {}'.format(client_ip))

  # check body
  try:
    validate(instance=body, schema=schema_walker)
    start_date = body['start_date']
    stop_date = body['stop_date']
    sampling_dt = body['sampling_dt']
    pg_closed = body['pg_closed']
    log_print('Parameters {} - {} sampling {} pg_closed len {}'.format(start_date, stop_date, sampling_dt, len(pg_closed)))
  except Exception as e:
    raise HTTPException(status_code=400, detail='invalid request payload : {}'.format(e))

  # init conf_walker
  try:
    cfg_w_file = os.path.join(os.environ['WORKSPACE'], 'minimocas', 'vars', 'conf_walker.json')
    with open(cfg_w_file) as cin: cfg_walker = json.load(cin)
    cw = conf_walker(cfg_walker)
  except Exception as e:
    log_print('conf_walker init failed : {}'.format(e))
    raise HTTPException(status_code=500, detail='conf_walker init : {}'.format(e))

  # generate sim config
  try:
    conf = cw.generate(start_date, stop_date)
  except Exception as e:
    log_print('walker config generation failed : {}'.format(e))
    raise HTTPException(status_code=500, detail='conf_walker generation failed : {}'.format(e))

  wdir = cw.wdir
  try:
    os.chdir(wdir)
  except:
    os.mkdir(wdir)
    os.chdir(wdir)

  conf['start_date'] = start_date
  conf['stop_date'] = stop_date
  conf['sampling_dt'] = sampling_dt
  conf['state_basename'] = wdir + '/walker'
  conf['dynamic_weights'] = {
    'mode'      : 'polygon_closed',
    'timetable' : pg_closed
  }

  confs = json.dumps(conf)

  with open(wdir + '/walker_response_conf.json', 'w') as outc: json.dump(conf, outc, indent=2)
  #print(confs, flush=True)

  s = simulation(confs)

  if s.is_valid():
    tsim = datetime.now()
    s.run()
    log_print('walker simulation done in {}'.format(datetime.now() - tsim))

    # collect output from local csv
    pof = s.polygon_outfile()
    #print('Polygon output file : {}'.format(pof))
    dfp = pd.read_csv(pof, sep = ';')
    pp = { ts : [ [ v[1], v[2], v[3] ] for v in df.values] for ts, df in dfp.groupby('timestamp')}
    ret = {
      'message' : 'simulation OK',
      'simulation_type' : 'walker',
      'data' : { 'polygon_population': pp }
      #'data' : { 'polygon_population': dfp.to_json(orient='values') }
    }
  else:
    raise HTTPException(status_code=501, detail='simulation init failed')
  return ret

@app.post("/sim_boat")
async def sim_boat_post(body: dict):
  # check body
  try:
    validate(instance=body, schema=schema_boat)
    start_date = body['start_date']
    stop_date = body['stop_date']
    sampling_dt = body['sampling_dt']
  except Exception as e:
    raise HTTPException(status_code=401, detail='invalid request payload : {}'.format(e))

  # init conf_boat
  try:
    cfg_b_file = os.path.join(os.environ['WORKSPACE'], 'minimocas', 'vars', 'conf_boat.json')
    with open(cfg_b_file) as cin: cfg_boat = json.load(cin)
    cb = conf_boat(cfg_boat)

    conf = cb.generate(start_date, stop_date)
  except Exception as e:
    log_print('boat config generation failed : {}'.format(e))
    raise HTTPException(status_code=500, detail='boat db query failed : {}'.format(e))

  wdir = cb.wdir
  try:
    os.chdir(wdir)
  except:
    os.mkdir(wdir)
    os.chdir(wdir)

  conf['start_date'] = start_date
  conf['stop_date'] = stop_date
  conf['sampling_dt'] = sampling_dt
  conf['state_basename'] = wdir + '/boat'

  confs = json.dumps(conf)

  with open(wdir + '/boat_response_conf.json', 'w') as outc: json.dump(conf, outc, indent=2)
  #print(confs, flush=True)

  s = simulation(confs)

  if s.is_valid():
    tsim = datetime.now()
    s.run()
    log_print('boat simulation done in {}'.format(datetime.now() - tsim))

    # response
    ret = {
      'message' : 'simulation OK',
      'simulation_type' : 'boat'
    }
  else:
    raise HTTPException(status_code=501, detail='simulation init failed')
  return ret
