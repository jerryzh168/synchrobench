import argparse
import re

def parse_args():
    parser = argparse.ArgumentParser(description='Process result...')
    parser.add_argument('-f', '--filename', action='store', type=str)
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
    
def main(args):
    filename = args.filename
    with open(filename, 'r') as handle:
        lines = handle.readlines()
    stats = dict()
    # get params
    for i in xrange(len(lines)):
        result = lines[i].split(':')
        if len(result) == 2:
            key, value = result
            stats[key.strip()] = value.strip()
    stats['Nb threads'] = int(stats['Nb threads'])
    to_persec(stats, '#txs')
    print stats
    return stats


if __name__ == '__main__':
    main(parse_args())
