import argparse
import re
import matplotlib.pyplot as plt

def parse_args():
    parser = argparse.ArgumentParser(description='Process result...')
    parser.add_argument('-f', '--filename', action='store', help='input file name', type=str)
    parser.add_argument('-e', '--enable_metrics', action='store_true', help='enable profile', type=bool, default=False)
    parser.add_argument('-m', '--metrics', nargs='+', help = 'list of metrics', required=False, type=list, default=[])
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
                nums = re.findall('\(' + metric + ', (\d+)\)', line)
                if len(nums) > 0:
                    pname = '#profile-' + metric
                    if pname in stats:
                        stats['#profile-' + metric].append(int(nums[0]))
                    else:
                        stats['#profile-' + metric] = [int(nums[0])]
                else:
                    print 'Metric:' + metric + 'Not found'

def main(args):
    filename = args.filename
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
    if args.enable_metrics:
        metrics = ['txs', 'mallocs', 'frees'] + args.metrics
        get_profile(stats, metrics)
        del stats['#profile']
        lines = []
        for metric in metrics:
            line, = plt.plot(stats['#profile-'+metric], label=metric)
            lines.append(line)
        plt.legend(metrics, loc=1)
        plt.savefig(args.filename + '.png')
    print args.filename, stats['#txs per second']
    return stats


if __name__ == '__main__':
    main(parse_args())
