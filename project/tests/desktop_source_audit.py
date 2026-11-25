#!/usr/bin/env python3
"""Audit private desktop source cores; report audio separately from catalog QA."""
import argparse,json,subprocess,os,sys
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('--build',type=Path,required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--sources',type=int,nargs='*');a=p.parse_args();assert os.path.ismount('/Volumes/SSD') and a.output.resolve().is_relative_to('/Volumes/SSD/Development');a.output.mkdir(exist_ok=True,parents=True)
r=a.build/'mac/resources';cat=r/'Content/catalog.json';data=json.loads(cat.read_text());issues=[];total=0;silence=[]
for i in a.sources or range(len(data['sources'])):
 log=a.output/f'source-{i}.jsonl'
 with log.open('w')as stream:
  run=subprocess.run([str(a.build/'mac/core-probe'),str(cat),str(r/'Content/Engines/mac'),str(i)],stdout=stream,stderr=subprocess.STDOUT,env={**os.environ,'TMPDIR':str(a.build/'mac/tmp')},timeout=1800)
 rows=[]
 for line in log.read_text().splitlines():
  try:rows.append(json.loads(line))
  except ValueError:pass
 expected=len(data['sources'][i]['sounds']);failed=[x for x in rows if x['nonfinite'] or x['restoreError']>1e-4 or x['sourceStateError']>1e-4]
 if run.returncode or len(rows)!=expected:issues.append(dict(source=i,code=run.returncode,rows=len(rows),expected=expected))
 issues+=failed;silence += [dict(source=i,program=x['program'],name=x['name'])for x in rows if x['rms']<1e-7];total+=len(rows)
 print(i,data['sources'][i]['name'],len(rows),len(failed),'issues',flush=True)
 (a.output/'summary.json').write_text(json.dumps(dict(complete=False,programs=total,issues=issues,silent_short_probes=silence),indent=2)+'\n')
(a.output/'summary.json').write_text(json.dumps(dict(complete=True,passed=not issues,programs=total,issues=issues,silent_short_probes=silence),indent=2)+'\n')
sys.exit(bool(issues))
