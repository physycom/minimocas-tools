#! /usr/bin/env python3

from http.server import BaseHTTPRequestHandler
from html.parser import HTMLParser
import json
import re
from datetime import datetime
import requests
import sys, os

# local path-independent import
sys.path.append(os.environ['WORKSPACE'] + '/minimocas/script')
import gtfs

datetime_format = '%Y-%m-%d %H:%M:%S'
def log_print(*args, **kwargs):
  print('[miniserver] {} | '.format(datetime.now().strftime(datetime_format)), end='', flush=True)
  print(*args, **kwargs, flush=True)

class Server(BaseHTTPRequestHandler):

  def log_message(self, format, *args):
    log_print('Request from {} details {}'.format(self.client_address[0], format%args))

  def do_HEAD(self):
    return

  def do_POST(self):
    return

  def do_GET(self):
    self.respond()

  def serve_html(self, filename):
    f = open(os.path.dirname(os.path.realpath(__file__)) + '/html/' + filename)
    status = 200
    content_type = 'text/html; charset=ISO-8859-1'
    response_content = f.read()
    response_content = bytes(response_content, 'UTF-8')
    size = len(response_content)
    return status, content_type, response_content, size

  def serve_404(self):
    status = 404
    content_type = 'text/plain'
    response_content = '404 Url not found.\n'
    response_content = bytes(response_content, 'UTF-8')
    size = len(response_content)
    return status, content_type, response_content, size

  def serve_json(self, geojson):
    status = 200
    content_type = 'application/json; charset=ISO-8859-1'
    response_content = json.dumps(geojson)
    response_content = bytes(response_content, 'UTF-8')
    size = len(response_content)
    return status, content_type, response_content, size

  def handle_http(self):
    if self.path.endswith('.html'):
      try:
        htmlfile = self.path.split('/')[-1]
        status, content_type, response_content, size = self.serve_html(htmlfile)
      except:
        status, content_type, response_content, size = self.serve_404()
    elif self.path.startswith('/gtfs'):
      data = self.prepare_gtfs()
      status, content_type, response_content, size = self.serve_json(data)
    elif self.path.startswith('/attractions'):
      data = self.prepare_attractions()
      status, content_type, response_content, size = self.serve_json(data)
    else:
      status, content_type, response_content, size = self.serve_404()

    self.send_response(status)
    self.send_header('Content-type', content_type)
    self.send_header('Content-length', size)
    self.end_headers()
    return response_content

  def respond(self):
    content = self.handle_http()
    self.wfile.write(content)

  def prepare_attractions(self):
    attr_file = '../test/work_release/attractions.json'
    try:
      with open(attr_file) as ain:
        attr = json.load(ain)
      log_print('Serving {} attractions'.format(len(attr['attractions'])))
      return attr
    except Exception as e:
      log_print('Prepare attractions failed : {}'.format(e))
      return []

  def prepare_gtfs(self):
    conf = {
      'base_url'       : 'http://actv.avmspa.it/sites/default/files/attachments/opendata/navigazione/',
      'data_dir'       : 'actv_data',
      'start_date'     : '2019-04-16 00:00',
      'stop_date'      : '2019-04-23 00:00',
      'lat_min'        : 45.418301,
      'lat_max'        : 45.451410,
      'lon_min'        : 12.296303,
      'lon_max'        : 12.372170
    }
    g = gtfs.gtfs_data(conf)
    g.get_remote_data()
    data = g.to_list(conf['start_date'], conf['stop_date'])

    params = self.path.split('?')
    if len(params) == 2:
      params = params[1].split('&')
    else:
      params = []
    params_dict = { p[0] : p[1] for p in [ p.split('=') for p in params ] }
    #print(params_dict)

    # apply filters
    if 'routeid_filter' in params_dict:
      rids = [ int(r) for r in params_dict['routeid_filter'].split(',') if r ]
      data_fil = [ d for d in data if d[1] in rids ]
    else:
      data_fil = data
    log_print('Serving {} trips'.format(len(data_fil)))
    return data_fil

if __name__ == '__main__':
  import argparse
  from http.server import HTTPServer

  parser = argparse.ArgumentParser()
  parser.add_argument('-a', '--server-address', help='http server address', default='localhost')
  parser.add_argument('-p', '--server-port', help='http server port', default=7777, type=int)
  args = parser.parse_args()

  HOST_NAME = args.server_address
  PORT_NUMBER = args.server_port

  httpd = HTTPServer((HOST_NAME, PORT_NUMBER), Server)
  log_print('Server UP {}:{}'.format(HOST_NAME, PORT_NUMBER))
  try:
    httpd.serve_forever()
  except KeyboardInterrupt:
    httpd.server_close()
    log_print('Server DOWN {}:{}'.format(HOST_NAME, PORT_NUMBER))
