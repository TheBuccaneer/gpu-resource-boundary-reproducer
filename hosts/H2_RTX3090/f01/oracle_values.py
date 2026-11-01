"""Independent mathematical oracle: no plan, C source, logger or file imports."""
MASK=4294967295
TAGS={'A1':1,'AO':10,'B1':2,'AS':30,'CI':3,'CO':11}
def initial(role,index):
 if role not in TAGS or type(index) is not int or not 0<=index<160:raise ValueError('unknown role/index')
 # Independent host formulation of the preregistered 32-bit canary relation.
 return (((index+1)*2654435769)&MASK)^((TAGS[role]*16909060)&MASK)^0xf010ca5e

def expected(role):
 result=[initial(role,i) for i in range(160)]
 if role in ('AO','CO'):
  source,salt=('A1',0x37a1b5c9) if role=='AO' else ('CI',0x68b2d7e1)
  result[16:80]=[initial(source,i)^salt for i in range(16,80)]
 return result

def foreign_output_word(index):return initial('B1',index)^0x37a1b5c9
