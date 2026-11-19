#!/usr/bin/env python3
"""Render every original source selection from the actual macOS source cores."""
from concurrent.futures import ThreadPoolExecutor,as_completed
from pathlib import Path
import json,os,subprocess,sys

assert os.path.ismount('/Volumes/SSD')
work=Path('/Volumes/SSD/Development/BuildCache/rhythmer-os/mpc3-native')
output=Path('/Volumes/SSD/Development/TestResults/rhythmer-engines-mac')
catalog=json.loads((work/'catalog.json').read_text())
def audit(index):
    command=[str(work/'mac/core-probe'),str(work/'catalog.json'),str(work/'mac/engines'),str(index)]
    log=output/f'source-{index}.jsonl'
    with log.open('w')as stream:
        result=subprocess.run(command,stdout=stream,stderr=subprocess.PIPE,text=True,timeout=1200,env={**os.environ,'TMPDIR':str(work/'mac')})
    rows=[json.loads(line)for line in log.read_text().splitlines()if line.startswith('{')]
    expected=len(catalog['sources'][index]['sounds'])
    errors=[]
    if result.returncode or len(rows)!=expected:errors.append(dict(source=index,returncode=result.returncode,expected=expected,actual=len(rows),error=result.stderr[-2000:]))
    for row in rows:
        if row['nonfinite'] or row['sourceStateError']>1e-4 or row['restoreError']>1e-4:errors.append(row)
    print(catalog['sources'][index]['name'],len(rows),'PASS'if not errors else'FAIL',flush=True)
    return rows,errors
all_rows=[];errors=[]
with ThreadPoolExecutor(max_workers=2)as pool:
    for future in as_completed([pool.submit(audit,index)for index in range(27)]):
        rows,failed=future.result();all_rows+=rows;errors+=failed
report=dict(passed=not errors,programs=len(all_rows),source_views=27,errors=errors,short_probe_silence=[r for r in all_rows if r['rms']<1e-7],rows=sorted(all_rows,key=lambda r:(r['source'],r['program'])))
(output/'mac-factory-audio.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps({k:v for k,v in report.items()if k!='rows'},indent=2),flush=True)
sys.exit(bool(errors))
