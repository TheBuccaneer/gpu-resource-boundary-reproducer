"""Independent values: no C, plan, logger or fixture imports. Frozen F02 v1."""
MASK=4294967295
TAGS={'A1':1,'A3':3,'O0':10,'O1':11,'O2':12}
NONCE=1408788826
SALT=3467527431
STAGES=(('O0:first','O0','A1'),('O0:reset','O0',None),('O0:second','O0','A1'),('O1:final','O1','A3'),('A1','A1',None),('A3','A3',None),('O2','O2',None))
def initial(role,index):
 if role not in TAGS or type(index) is not int or not 0<=index<160:raise ValueError('bad role/index')
 return (((index+1)*2654435769)&MASK)^((TAGS[role]*16909060)&MASK)^NONCE
def expected(stage):
 role,source=next((r,s) for label,r,s in STAGES if label==stage)
 return [(initial(source,i)^SALT) if source and 16<=i<80 else initial(role,i) for i in range(160)]
