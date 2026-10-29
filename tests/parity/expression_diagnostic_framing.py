"""Translate native diagnostic presentation to original source framing.

This adapter consumes the actual native renderings. It neither parses raw panic
fields nor repairs missing delimiters, broken-point placement or root arguments.
"""
def method(prefix,name):
    result=prefix;upper=bool(prefix)
    for c in name:
        if c=="_":upper=True
        else:
            result+=chr(ord(c)-32) if upper and "a"<=c<="z" else c
            upper=False
    return result
def quoted(text):return '"'+text+'"'
def array(parts):return '['+', '.join(quoted(x) for x in parts)+']'
def source_entry(name):
    result=''
    for i,c in enumerate(name):
        if c.isupper():
            if i:result+='_' 
            result+=chr(ord(c)+32) if 'A'<=c<='Z' else c
        else:result+=c
    return result
def original_expression(value):
    # Consume getter tokens from the end so identical strings in root arguments
    # or opaque field names cannot be globally replaced as executable methods.
    text=value['original_expr_with_broken_point']; suffix=[]
    for part in reversed(value['access_path']):
        broken=text.endswith('<broken>')
        if broken:text=text[:-len('<broken>')]
        token='.'+method('get',part)+'()'
        if not text.endswith(token):
            raise AssertionError(('native getter framing differs', token, text))
        text=text[:-len(token)]
        suffix.append('.get_'+part+'()'+('<broken>' if broken else ''))
    root_name=value['root'].split('(')[0]
    entry=method('',root_name)
    if entry and 'A'<=entry[0]<='Z':entry=entry[0].lower()+entry[1:]
    prefix='E::'+entry+'('
    if not text.startswith(prefix):
        raise AssertionError(('native root entry differs', prefix, text))
    return 'E::'+source_entry(root_name)+'('+text[len(prefix):]+''.join(reversed(suffix))
def nested_selection(value):
    text=value['nested_fix']
    if text is None:return 'null'
    parts=value['access_path'];node=value['break_point']
    try:start=parts.index(node)
    except ValueError:start=0
    position=0
    def consume(token):
        nonlocal position
        if not text.startswith(token, position):
            raise AssertionError(('native selection framing differs', token, text[position:]))
        position+=len(token)
    consume(method('select',node)+'With(')
    source='select_'+node+'('
    for i in range(start+1,len(parts)):
        placeholder='<generated Q entry for '+parts[i-1]+'>.'
        consume(placeholder);source+=placeholder
        leaf=i+1==len(parts)
        consume(method('select',parts[i])+('()' if leaf else 'With('))
        source+='select_'+parts[i]+('()' if leaf else '(')
    remainder=text[position:]
    if not (remainder and set(remainder) == {')'}):
        raise AssertionError(('native closing delimiters differ', remainder))
    return quoted(source+remainder)
def to_source(value):
    if not set(value) == {'original_expr_with_broken_point', 'root', 'access_path', 'break_point', 'missing_preload', 'suggested_fix', 'nested_fix', 'severity', 'human_message'}:
        raise AssertionError('native diagnostic fields differ')
    node=value['break_point']
    if not value['suggested_fix'] == method('select', node) + '()':
        raise AssertionError('native leaf selection differs')
    original=quoted(original_expression(value))
    suggested=quoted('select_'+node+'()')
    nested=nested_selection(value)
    root_name=value['root'].split('(')[0]
    attempted='.'.join(value['access_path'])
    english='Access to '+root_name+'.'+attempted+' requires preloading. Add '+value['suggested_fix']
    if value['nested_fix'] is not None:english+=' or the complete nested selection '+value['nested_fix']
    if not value['human_message'] == english + ' to the query.':
        raise AssertionError('native English message differs')
    human=quoted('访问 '+root_name+'.'+attempted+' 时缺少预加载。请在查询中加入 '+suggested+(' 或完整嵌套加载 '+nested if nested!='null' else ''))
    fields=[('original_expr_with_broken_point',original),('root',value['root']),
        ('access_path',array(value['access_path'])),('break_point',quoted(node)),
        ('missing_preload',array(value['missing_preload'])),('suggested_fix',suggested),
        ('nested_fix',nested),('severity',quoted(value['severity'])),('human_message',human)]
    return '\n\n💥 [Coding Logic Bug]\n\n'+''.join(name+': '+text+'\n' for name,text in fields)

def verify_negative_controls(value,original):
    import copy
    if not to_source(value) == original:
        raise AssertionError('positive frame control differs')
    mutations={
        'missing_broken_point':lambda x:x.update(original_expr_with_broken_point=x['original_expr_with_broken_point'].replace('<broken>','')),
        'wrong_root_arguments':lambda x:x.update(original_expr_with_broken_point=x['original_expr_with_broken_point'].replace('id=7','id=8')),
        'wrong_missing_preload':lambda x:x['missing_preload'].append('fake'),
        'wrong_severity':lambda x:x.update(severity='warning'),
        'wrong_leaf_suggestion':lambda x:x.update(suggested_fix='selectFake()'),
        'wrong_english_message':lambda x:x.update(human_message='silently corrected'),
        'extra_native_field':lambda x:x.update(invented_field='fake')}
    for name,mutate in mutations.items():
        corrupted=copy.deepcopy(value);mutate(corrupted)
        try:rejected=to_source(corrupted)!=original
        except (AssertionError,KeyError):rejected=True
        if not rejected:
            raise AssertionError('presentation adapter repaired ' + name)
    return len(mutations)
