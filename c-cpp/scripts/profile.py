import argparse
import re
import matplotlib.pyplot as plt
import os
import collections

def parse_args():
    parser = argparse.ArgumentParser(description='Process result...')
    parser.add_argument('-f', '--filename', action='store', help='input file name', type=str)
    parser.add_argument('-d', '--directory', action='store', help='result directory', type=str)
    parser.add_argument('-e', '--enable_metrics', action='store_true', help='enable profile', default=False)
    parser.add_argument('-m', '--metrics', nargs='+', help = 'list of metrics', required=False, type=str, default=[])
    return parser.parse_args()

def get_persec(s):
    per_sec = re.findall('\d+ \((\d+\.\d+) / s\)', s)
    if len(per_sec) > 0:
        return float(per_sec[0])
    return None

def to_persec(stats, key):
    if key in stats:
        per_sec = get_persec(stats[key])
        stats[key + ' per second'] = per_sec

def get_profile(stats, metrics):
    if '#profile' in stats:
        lines = stats['#profile']
        for line in lines:
            for metric in metrics:
                if metric == 'memory_pressure':
                    mallocs = re.findall('\(' + 'mallocs' + ', (\d+)\)', line)
                    frees = re.findall('\(' + 'frees' + ', (\d+)\)', line)
                    nums = float(mallocs[0]) - float(frees[0])
                else:
                    nums = re.findall('\(' + metric + ', (\d+)\)', line)[0]
                pname = '#profile-' + metric
                if pname in stats:
                    stats['#profile-' + metric].append(int(nums))
                else:
                    stats['#profile-' + metric] = [int(nums)]

def process_result(filename, enable_metrics=False, metrics=[]):
    print 'filename', filename
    with open(filename, 'r') as handle:
        lines = handle.readlines()
    stats = dict()
    # get params
    # TODO add more reporting options
    for i in xrange(len(lines)):
        result = lines[i].split(':')

        if len(result) == 2:
            key, value = result
            k = key.strip()

            if k in stats:
                if type(stats[k]) == type([]):
                    stats[k].append(value.strip())
                else:
                    stats[k] = [stats[k], value.strip()]
            else:
                stats[key.strip()] = value.strip()
    stats['Nb threads'] = int(stats['Nb threads'])
    to_persec(stats, '#txs')
    if enable_metrics:
        get_profile(stats, metrics)
        del stats['#profile']
        lines = []
        for metric in metrics:
            line, = plt.plot(stats['#profile-'+metric], label=metric)
            lines.append(line)
        plt.legend(metrics, loc=1)
        plt.savefig(filename + '.png')
    # print args.filename, stats['#txs per second'], stats['#mallocs'], \
    #    stats['#frees'], stats['#memory pressure']
    return stats



def atoi(text):
    return int(text) if text.isdigit() else text

def natural_keys(text):
    '''
    alist.sort(key=natural_keys) sorts in human order
    http://nedbatchelder.com/blog/200712/human_sorting.html
    (See Toothy's implementation in the comments)
    '''
    return [ atoi(c) for c in re.split('(\d+)', text) ]

def main(args):
    d = args.directory
    files = os.listdir(d)
    filemap = collections.defaultdict(list)
    for f in files:
        test = f[:-2]
        filemap[test].append(f)
    results = collections.defaultdict(dict)
    metrics = ['#txs per second', '#mallocs', '#frees', '#memory pressure']
    for f, runs in filemap.items():
        num_runs = len(runs)
        for run in runs:
            stats = process_result(args.directory+run, args.enable_metrics, args.metrics)
            for metric in metrics:
                if metric not in results[f]:
                    results[f][metric] = [float(stats[metric])]
                else:
                    results[f][metric].append(float(stats[metric]))
        for metric in metrics:
            results[f][metric] = sum(results[f][metric]) / float(num_runs)
    results = results.items()
    results.sort(key=lambda x:natural_keys(x[0]))
    for test, res in results:
        show = [test]
        for metric in metrics:
            show.append(str(res[metric]))
        print ','.join(show)



if __name__ == '__main__':
    main(parse_args())
