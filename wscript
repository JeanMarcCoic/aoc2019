import os
import waflib

out = '.build-waf-%s' % os.environ.get('P', 'default')

def options(ctx):
    ctx.recurse('lib-common')

def configure(ctx):
    ctx.recurse('lib-common')

def build(ctx):
    ctx.recurse(['lib-common', 'src'])
