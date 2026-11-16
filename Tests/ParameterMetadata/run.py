#!/usr/bin/env python3
"""Build/prepare by default; --execute explicitly runs only the isolated VST3 host."""
from pathlib import Path
import argparse
import hashlib
import json
import os
import plistlib
import struct
import subprocess
import sys
import xml.etree.ElementTree as ET

HERE = Path(__file__).resolve().parent
NAMES = ['held_float', 'initial_integer', 'initial_curve', 'late_default', 'late_range', 'late_curve']
STAGES = ['initial', 'late-default-only', 'late-range-only', 'late-curve-only', 'noop-range', 'late-create-no-default']


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def command(args, **kwargs):
    return subprocess.run([str(x) for x in args], check=True, **kwargs)


def git(source, *args):
    return subprocess.check_output(['git', '-C', str(source), *args], text=True).strip()


def f32(value):
    return struct.unpack('<f', struct.pack('<f', float(value)))[0]


def read_state(path):
    data = path.read_bytes()
    offset = 4
    for _ in range(struct.unpack_from('<i', data)[0]):
        offset = data.index(b'\0', offset) + 1
        offset = data.index(b'\0', offset) + 1
    offset += 16
    magic, length = struct.unpack_from('<II', data, offset)
    assert magic == 0x21324356, 'Unexpected plugdata XML state envelope'
    return ET.fromstring(data[offset + 8:offset + 8 + length])


def write_state(path, node):
    location = node.find('Patches')[0].get('Location')
    xml = ET.tostring(node, encoding='utf-8')
    payload = struct.pack('<II', 0x21324356, len(xml)) + xml + b'\0'
    path.write_bytes(struct.pack('<i', 1) + b'\0' + location.encode() + b'\0'
                     + struct.pack('<iifi', 0, 0, 0, len(payload)) + payload)


def fixture(path):
    objects, connections = [], []

    def add(kind, value):
        index = len(objects)
        objects.append(f'#X {kind} {30 + (index % 6) * 200} {30 + (index // 6) * 70} '
                       + value.replace(',', ' \\,') + ';')
        return index

    def connect(a, b, outlet=0):
        connections.append(f'#X connect {a} {outlet} {b} 0;')

    load = add('obj', 'loadbang')
    trigger = add('obj', 't b b b b b b')
    connect(load, trigger)
    messages = ['create 7.5 0 10', 'create 3 0 4, mode 2', 'create 4 0 40, mode 4',
                'create 1 0 4, mode 2', 'create 2 0 4, mode 2', 'create 4 0 40']
    params = {}
    for i, (name, msg) in enumerate(zip(NAMES, messages)):
        box = add('msg', msg)
        params[name] = add('obj', f'param {name} 0')
        connect(trigger, box, len(NAMES) - 1 - i)
        connect(box, params[name])
    params['no_default'] = add('obj', 'param no_default 0')
    for ms, name, msg in [(250, 'late_default', 'create 3 0 4'),
                          (600, 'late_range', 'range 0 8'),
                          (900, 'late_curve', 'mode 4'),
                          (1250, 'late_range', 'range 0 8'),
                          (1600, 'no_default', 'create')]:
        delay = add('obj', f'del {ms}')
        box = add('msg', msg)
        connect(load, delay)
        connect(delay, box)
        connect(box, params[name])
    path.write_text('#N canvas 0 50 1350 850 10;\n' + '\n'.join(objects + connections) + '\n')


def build(source):
    candidates = list((source / 'Libraries/JUCE/modules').glob('*/format_types/VST3_SDK'))
    assert len(candidates) == 1, candidates
    sdk = candidates[0]
    dest = HERE / 'build'
    dest.mkdir(exist_ok=True)
    library = dest / 'libprobe_paths.dylib'
    executable = dest / 'probe'
    fingerprint = {'sdk': str(sdk), 'probe': sha(HERE / 'probe.mm'),
                   'interposer': sha(HERE / 'probe_paths.mm'),
                   'juce': git(source / 'Libraries/JUCE', 'rev-parse', 'HEAD')}
    stamp = dest / 'build.json'
    if not (stamp.exists() and json.loads(stamp.read_text()) == fingerprint
            and executable.exists() and library.exists()):
        with (dest / 'build.log').open('w') as log:
            commands = [
                ['clang++', '-arch', 'arm64', '-O2', '-std=c++17', '-dynamiclib',
                 HERE / 'probe_paths.mm', '-framework', 'Foundation',
                 '-Wl,-install_name,@rpath/libprobe_paths.dylib', '-o', library],
                ['clang++', '-arch', 'arm64', '-O2', '-std=c++17', '-DNDEBUG',
                 '-I' + str(sdk), HERE / 'probe.mm', sdk / 'pluginterfaces/base/funknown.cpp',
                 sdk / 'pluginterfaces/base/coreiids.cpp', sdk / 'public.sdk/source/vst/vstinitiids.cpp',
                 '-framework', 'Cocoa', '-Wl,-rpath,@executable_path',
                 '-Wl,-needed_library,' + str(library), '-o', executable]]
            for cmd in commands:
                log.write(' '.join(str(x) for x in cmd) + '\n')
                log.flush()
                command(cmd, stdout=log, stderr=log)
        stamp.write_text(json.dumps(fingerprint, indent=2) + '\n')
    return executable, library


def sandbox(output):
    runtime = output / 'runtime'
    home = runtime / 'home'
    temp = runtime / 'temp'
    for p in [home / 'Documents', home / 'Library/Caches', temp]:
        p.mkdir(parents=True, exist_ok=True)
    env = dict(os.environ)
    env['PLUGDATA_PARAMETER_TEST_HOME'] = str(home)
    env['PLUGDATA_PARAMETER_TEST_TMPDIR'] = str(temp)
    profile = output / 'isolation.sb'
    profile.write_text('(version 1)\n(allow default)\n(deny file-write*)\n'
                       '(allow file-write* (subpath ' + json.dumps(str(output)) + '))\n'
                       '(allow file-write-data (literal "/dev/null"))\n'
                       '(deny network*)\n')
    return profile, env


def prepare_case(source, output, recall=None):
    output.mkdir(parents=True, exist_ok=True)
    abstraction = source / 'Resources/Patches/param.pd'
    head = subprocess.check_output(['git', '-C', str(source), 'show', 'HEAD:Resources/Patches/param.pd'])
    assert abstraction.read_bytes() == head, 'Standard abstraction differs from source HEAD'
    (output / 'param.pd').write_bytes(head)
    patch = output / 'fixture.pd'
    fixture(patch)
    if recall:
        node = read_state(recall)
        for p in node.findall('PARAM'):
            if p.get('enabled') == '1':
                p.set('value', str(f32(.8)))
        node.find('Patches')[0].set('Content', '')
        node.find('Patches')[0].set('Location', str(patch))
    else:
        node = ET.Element('plugdata_save', Version='0.9.3', Oversampling='0', Latency='0', TailLength='0', Legacy='0')
        ET.SubElement(node, 'PARAM', id='volume', value='0.8')
        ET.SubElement(ET.SubElement(node, 'Patches'), 'Patch', Content='', Location=str(patch),
                      SplitIndex='0', PluginMode='0', PluginModeScale='100')
    write_state(output / 'input.state', node)
    return sandbox(output)


def execute_case(source, bundle, output, executable, order, pending=False, recall=None):
    if (output / 'result.json').exists():
        raise RuntimeError(f'Refusing to overwrite completed result: {output}')
    profile, env = prepare_case(source, output, recall)
    prefix = output / 'probe'
    cmd = ['/usr/bin/sandbox-exec', '-f', str(profile), str(executable), str(bundle),
           str(output / 'input.state'), str(prefix), order]
    if pending:
        cmd.append('--pending-automation')
    with (output / 'raw.log').open('w') as log:
        command(cmd, env=env, stdout=log, stderr=log, timeout=180)
    report = json.loads(prefix.with_suffix('.json').read_text())
    report['states'] = {}
    for path in sorted(output.glob('probe-*.state')):
        node = read_state(path)
        path.with_suffix('.xml').write_bytes(ET.tostring(node))
        report['states'][path.stem.removeprefix('probe-')] = [dict(p.attrib) for p in node.findall('PARAM')
                                                             if p.get('enabled') == '1']
    plist = plistlib.loads((bundle / 'Contents/Info.plist').read_bytes())
    binary = bundle / 'Contents/MacOS' / plist['CFBundleExecutable']
    report['identity'] = {'bundle': str(bundle), 'binary_sha256': sha(binary), 'source': str(source),
                          'source_commit_when_tested': git(source, 'rev-parse', 'HEAD'),
                          'provenance_note': 'Checkout observed when tested; binary build provenance is recorded separately in task manifests.',
                          'source_status_when_tested': git(source, 'status', '--short'),
                          'juce_commit': git(source / 'Libraries/JUCE', 'rev-parse', 'HEAD'),
                          'juce_status_when_tested': git(source / 'Libraries/JUCE', 'status', '--short'),
                          'host_sha256': sha(executable), 'host_source_sha256': sha(HERE / 'probe.mm'),
                          'path_interposer_source_sha256': sha(HERE / 'probe_paths.mm'),
                          'fixture_sha256': sha(output / 'fixture.pd'),
                          'param_sha256': sha(output / 'param.pd'), 'command': cmd,
                          'recall_source': str(recall) if recall else None}
    report['pending'] = pending
    report['recall'] = bool(recall)
    (output / 'result.json').write_text(json.dumps(report, indent=2) + '\n')
    return report


def validate(reports, expectation, baseline=None):
    checks = []

    def check(name, actual, wanted, category='invariant'):
        checks.append({'name': name, 'pass': actual == wanted, 'actual': actual,
                       'expected': wanted, 'category': category})

    for key, report in reports.items():
        stages = {s['stage']: s for s in report['snapshots']}

        def param(stage, name):
            return next(p for p in stages[stage]['parameters'] if p['title'] == name)

        def state(stage):
            return {p['name']: p for p in report['states']['final' if stage == 'controller-recreated' else stage]}

        ids = [p['id'] for p in stages['initial']['parameters']]
        for stage, snap in stages.items():
            check(key + '/' + stage + '/IDs', [p['id'] for p in snap['parameters']], ids)
            check(key + '/' + stage + '/gestures', [snap['begins'], snap['ends']], [0, 0])
            expected_names = NAMES + (['no_default'] if stage in ['late-create-no-default', 'controller-recreated'] else [])
            check(key + '/' + stage + '/names', sorted(state(stage)), sorted(expected_names))
            for name, saved in state(stage).items():
                expected = f32(saved['value'])
                if report['pending'] and name == 'held_float' and stage not in ['initial', 'controller-recreated']:
                    expected = .9
                    check(key + '/' + stage + '/pending processor value', f32(saved['value']), .75)
                category = 'mapping' if name in ['initial_curve', 'late_range', 'late_curve'] and stage != 'controller-recreated' else 'invariant'
                check(key + '/' + stage + '/' + name + '/exact current', param(stage, name)['current'], expected, category)
            check(key + '/' + stage + '/integer steps', param(stage, 'initial_integer')['step_count'], 4)
            check(key + '/' + stage + '/integer default', param(stage, 'initial_integer')['default'], .75)
        for before, after, name in [('initial', 'late-default-only', 'late_default'),
                                    ('late-default-only', 'late-range-only', 'late_range')]:
            a, b = stages[before], stages[after]
            delta = b['restarts'][len(a['restarts']):]
            check(key + '/' + after + '/info restart', any(v & 16 for v in delta), True, 'notification')
        check(key + '/late default metadata', param('late-default-only', 'late_default')['default'], .75, 'notification')
        check(key + '/late range steps', param('late-range-only', 'late_range')['step_count'], 8, 'notification')
        for before, after, name in [('late-default-only', 'late-range-only', 'late_range'),
                                    ('late-range-only', 'late-curve-only', 'late_curve')]:
            a, b = stages[before], stages[after]
            delta = b['restarts'][len(a['restarts']):]
            check(key + '/' + after + '/value restart', any(v & 4 for v in delta), True, 'mapping')
            check(key + '/' + after + '/no user edit', b['edits'], a['edits'])
            # Decode raw values from the saved normalization using the declared
            # mapping. Float32 inverse arithmetic may round, so this one derived
            # quantity uses a tight tolerance; normalized consistency above is exact.
            def raw(saved):
                skew = {1: 1, 2: 1, 3: 4, 4: .25}[int(saved['mode'])]
                lo, hi = float(saved['min']), float(saved['max'])
                return lo + (hi - lo) * float(saved['value']) ** (1 / skew)
            left, right = raw(state(before)[name]), raw(state(after)[name])
            check(key + '/' + after + '/raw value retained', abs(left - right) < 2e-5, True)
        a, b = stages['late-curve-only'], stages['noop-range']
        delta = b['restarts'][len(a['restarts']):]
        check(key + '/no-op no values restart', any(v & 4 for v in delta), False)
        check(key + '/no-op no edit', b['edits'], a['edits'])
        check(key + '/no-op controller identical', b['parameters'], a['parameters'])
        a, b = stages['noop-range'], stages['late-create-no-default']
        delta = b['restarts'][len(a['restarts']):]
        check(key + '/create without default info restart', any(v & 16 for v in delta), True)
        check(key + '/create without default current', param('late-create-no-default', 'no_default')['current'], 0.0)
        if report['recall']:
            for stage in stages:
                for name in ['held_float', 'initial_integer', 'initial_curve', 'late_default']:
                    check(key + '/' + stage + '/' + name + '/recall preserved', f32(state(stage)[name]['value']), f32(.8))
        if baseline:
            reference = json.loads((baseline / key / 'result.json').read_text())
            check(key + '/processor state identical to baseline', report['states'], reference['states'])
    failures = [c for c in checks if not c['pass']]
    allowed = {'baseline': {'notification', 'mapping'}, 'notifications': {'mapping'}, 'fixed': set()}[expectation]
    unexpected = [c for c in failures if c['category'] not in allowed]
    observed = {c['category'] for c in failures}
    missing = allowed - observed
    return {'expectation': expectation, 'runs': len(reports), 'checks': checks,
            'passed': len(checks) - len(failures), 'failed': len(failures),
            'unexpected_failures': unexpected, 'missing_expected_failure_categories': sorted(missing),
            'matches_expectation': not unexpected and not missing,
            'scope': 'Native arm64 VST3 API only; offline silent buffers; no DAW/editor/audio device/listening.'}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, required=True)
    parser.add_argument('--bundle', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--expect', choices=['baseline', 'notifications', 'fixed'], default='fixed')
    parser.add_argument('--baseline', type=Path, help='Clean baseline output for exact processor-state comparison')
    parser.add_argument('--execute', action='store_true', help='Explicitly permit plug-in execution after isolation self-test')
    parser.add_argument('--self-test-isolation', action='store_true', help='Run path/write isolation check only, never load a plug-in')
    parser.add_argument('--validate-only', action='store_true')
    args = parser.parse_args()
    source, output = args.source.resolve(), args.output.resolve()
    if not output.is_relative_to(HERE) or output == HERE:
        parser.error('--output must be a child directory of this tests directory')
    output.mkdir(parents=True, exist_ok=True)
    if args.validate_only:
        reports = {p.name: json.loads((p / 'result.json').read_text()) for p in output.iterdir()
                   if p.is_dir() and (p / 'result.json').exists()}
        assert len(reports) == 5, 'Expected all five native runs'
    else:
        executable, _ = build(source)
        isolation = output / 'isolation-check'
        isolation.mkdir(exist_ok=True)
        profile, env = sandbox(isolation)
        # Deliberately target a location still inside the tests workspace but
        # outside the child's allowed output. It must remain uncreated.
        denied = output / 'must-not-be-created-by-host'
        isolation_cmd = ['/usr/bin/sandbox-exec', '-f', str(profile), str(executable),
                         '--verify-isolation', str(denied)]
        (output / 'isolation-command.json').write_text(json.dumps({'command': isolation_cmd,
            'env_overrides': {k: v for k, v in env.items() if k.startswith('PLUGDATA_PARAMETER_TEST_')}}, indent=2) + '\n')
        if args.self_test_isolation or args.execute:
            result = subprocess.check_output(isolation_cmd, env=env, text=True, timeout=30)
            (output / 'isolation-result.json').write_text(result)
            assert json.loads(result)['pass'] and not denied.exists()
        if args.self_test_isolation:
            print((output / 'isolation-result.json').read_text())
            return
        if not args.execute:
            prepare_case(source, output / 'prepared-example')
            print(f'Built host and prepared fixture/profile in {output}; no plug-in was executed.')
            return
        if not args.bundle:
            parser.error('--bundle is required with --execute')
        bundle = args.bundle.resolve()
        reports = {}
        cases = [('fresh-controller-first', 'controller-first', False, False),
                 ('fresh-state-first', 'state-first', False, False),
                 ('recall-controller-first', 'controller-first', True, False),
                 ('recall-state-first', 'state-first', True, False),
                 ('pending-controller-first', 'controller-first', False, True)]
        for key, order, recall, pending in cases:
            print('Running', key, flush=True)
            recalled = output / 'fresh-state-first/probe-initial.state' if recall else None
            reports[key] = execute_case(source, bundle, output / key, executable, order, pending, recalled)
    result = validate(reports, args.expect, args.baseline.resolve() if args.baseline else None)
    (output / 'regression.json').write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps({k: v for k, v in result.items() if k not in ['checks', 'unexpected_failures']}, indent=2))
    if not result['matches_expectation']:
        print(json.dumps(result['unexpected_failures'], indent=2))
        raise SystemExit(1)


if __name__ == '__main__':
    main()
