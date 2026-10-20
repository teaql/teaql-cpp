"""Public typed-LIKE evaluation and read-result pipeline input matrix."""
import itertools


def cases():
    output=[]
    patterns=['','%','_','a%','\\%','雪_\0']
    originals=['','private\0雪','o'*4096]
    texts=[None,'','abc','a_c','a%c','雪😀\0','\\%','true']
    for pattern,original,text,operator in itertools.product(patterns,originals,texts,range(12)):
        output.append(dict(kind='pattern',pattern=pattern,original=original,text=text,wrapper=4,operator=operator))
    for pattern,original,wrapper in itertools.product(patterns,originals,range(4)):
        output.append(dict(kind='pattern',pattern=pattern,original=original,text=None,wrapper=wrapper,operator=0))
    for profile,mode,rich,count,borrowed in itertools.product(range(10),range(5),(False,True),(0,3),(False,True)):
        output.append(dict(kind='execution',profile=profile,mode=mode,rich=rich,count=count,borrowed=borrowed))
    return output
