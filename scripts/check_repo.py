"""Check portable docs, matching versions, source provenance and budget arithmetic."""
import hashlib
import json
from pathlib import Path
import re
from urllib.parse import unquote, urlsplit

ROOT=Path(__file__).resolve().parents[1]
version=(ROOT/'VERSION').read_text(encoding='utf-8-sig').strip()
assert re.fullmatch(r'\d+\.\d+\.\d+',version)
assert f'"{version}"' in (ROOT/'include/version.h').read_text()
ref=json.loads((ROOT/'mechanical/reference.json').read_text(encoding='utf-8-sig'))
assert ref['version']==version
assert version in (ROOT/'README.md').read_text(encoding='utf-8-sig')
report=json.loads((ROOT/'mechanical/validation.json').read_text())
assert report['version']==version
for name,expected in report['source_sha256'].items():
    content=(ROOT/'mechanical'/name).read_text(encoding='utf-8-sig').encode('utf-8')
    assert hashlib.sha256(content).hexdigest()==expected, f'Rerun mechanical/validate.py after editing {name}'
bom=json.loads((ROOT/'docs/bom.json').read_text())
parts=round(sum(i['unit']*i['quantity'] for i in bom['items']),2)
assert parts+bom['shipping_allowance']<=bom['ceiling']
for path in [ROOT/'README.md',ROOT/'CHANGELOG.md',ROOT/'CONTRIBUTING.md',*sorted((ROOT/'docs').glob('*.md'))]:
    for raw in re.findall(r'!?\[[^\]]*\]\(([^)]+)\)',path.read_text(encoding='utf-8-sig')):
        target=raw.split()[0].strip('<>')
        if target.startswith(('#','https://','http://','mailto:')):continue
        local=unquote(urlsplit(target).path)
        assert (path.parent/local).exists(),f'Broken link in {path.name}: {local}'
print(f'PASS: v{version}; source hashes; internal links; THB {parts:.2f} + {bom["shipping_allowance"]:.2f} delivery <= {bom["ceiling"]}')
