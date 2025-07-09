from building import *
import os

cwd     = GetCurrentDir()
src = Glob('src/*.c')
path = [cwd + '/src']

group = DefineGroup('Thread-Metric', src, depend = [''], CPPPATH = path)

Return('group')
