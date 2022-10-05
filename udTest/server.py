#!/usr/bin/env python3
from http.server import HTTPServer, SimpleHTTPRequestHandler, test
import sys

class CORSRequestHandler (SimpleHTTPRequestHandler):
	extensions_map={
		'.wasm': 'application/wasm',
		'.html': 'text/html',
		'.js':	'application/x-javascript',
		'.css':	'text/css',
		'': 'application/octet-stream', # Default
	}
	def end_headers (self):
		self.send_header('Cross-Origin-Embedder-Policy', 'require-corp')
		self.send_header('Cross-Origin-Opener-Policy', 'same-origin')
		SimpleHTTPRequestHandler.end_headers(self)

if __name__ == '__main__':
	test(CORSRequestHandler, HTTPServer, port=int(sys.argv[1]) if len(sys.argv) > 1 else 8000)
