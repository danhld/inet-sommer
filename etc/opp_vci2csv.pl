#!/usr/bin/env perl
#
# opp_vci2csv.pl - Outputs OMNeT++ 4 output vector-index files in CSV format
#
# Copyright (C) 2009 Christoph Sommer <christoph.sommer@informatik.uni-erlangen.de>
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
#
use strict;
use warnings;
use Getopt::Long;
use List::Util qw[min max];

my $modulesRe = "";
GetOptions (
	"modules|m:s" => \$modulesRe,
);

if (@ARGV < 0) {
	print STDERR "usage: vci2csv.pl [OPTIONS]\n";
	print STDERR "\n";
	print STDERR "  -m --modules:      Regular expression to match module names against. If used\n";
	print STDERR "                     with a named capture group (?<module>...), only this portion\n";
	print STDERR "                     of the module name is considered\n";
	print STDERR "                     [default: all]\n";
	print STDERR "\n";
	print STDERR "e.g.: vci2csv.pl -m '".'^scenario\.host\[(?<module>[0-9]+)\]'."' <input.vec >output.csv\n";
	exit;
}

#my $configname; # from header
#my $runnumber; # from header
#my $repetition; # from header
my %nodeandname_to_id; # node/name mapping to aggregate multiple different vectors
my %id_to_id; # id mapping to aggregate multiple different vectors
my %id_to_node; # from vector def
my %id_to_name; # from vector def
my %id_to_type; # from vector def
my %id_to_firstTime;
my %id_to_lastTime;
my %id_to_count;
my %id_to_min;
my %id_to_max;
my %id_to_sum;
my %id_to_sumSqr;

while (<>) {
	# line introduces new vector
	if (m{
			^vector
			\s+
			(?<vecnum>[0-9.]+)
			\s+
			(("(?<nodname1>[^"]+)")|(?<nodname2>[^\s]+))
			\s+
			(("(?<vecname1>[^"]+)")|(?<vecname2>[^\s]+))
			(\s+(?<vectype>[ETV]+))?
			\r?\n$
			}x) {
		my $id = $+{vecnum};
		my $node = defined($+{nodname1})?$+{nodname1}:"" . defined($+{nodname2})?$+{nodname2}:"";
		my $name = defined($+{vecname1})?$+{vecname1}:"" . defined($+{vecname2})?$+{vecname2}:"";
		my $type = defined($+{vectype})?$+{vectype}:"";
		if (defined($modulesRe) and ($modulesRe)) {
			next unless ($node =~ $modulesRe);
			if (defined($+{module})) {
				$node = $+{module};
			}
		}
		my $nodeandname = $node.'/'.$name;
		if (defined($nodeandname_to_id{$nodeandname})) {
			$id_to_id{$id} = $nodeandname_to_id{$nodeandname};
			$id = $nodeandname_to_id{$nodeandname};
		} else {
			$id_to_id{$id} = $id;
			$nodeandname_to_id{$nodeandname} = $id;
		}
		$id_to_node{$id} = $node;
		$id_to_name{$id} = $name;
		$id_to_type{$id} = $type;
		next;
	}

	# line must contain vector data
	next unless (m{^(?<vecnum>[0-9]+)\s+(?<vecdata>.*)\r?\n$}x);

	my $vecnum =$+{vecnum};

	# vector must have been introduced
	next unless defined($id_to_id{$vecnum});
	$vecnum = $id_to_id{$vecnum};

	my $node = $id_to_node{$vecnum};
	my $name = $id_to_name{$vecnum};
	my $type = $id_to_type{$vecnum};


	# extract vector-index data
	unless ($+{vecdata} =~ m{
				^
				(?<offset>[0-9]+)
				\s+
				(?<size>[0-9]+)
				\s+
				(?<startEventNum>[0-9]+)
				\s+
				(?<endEventNum>[0-9]+)
				\s+
				(?<startTime>[0-9e.+-]+)  # allow -1.234e+56
				\s+
				(?<endTime>[0-9e.+-]+)  # allow -1.234e+56
				\s+
				(?<count>[0-9]+)
				\s+
				(?<min>[0-9e.+-]+)  # allow -1.234e+56
				\s+
				(?<max>[0-9e.+-]+)  # allow -1.234e+56
				\s+
				(?<sum>[0-9e.+-]+)  # allow -1.234e+56
				\s+
				(?<sumSqr>[0-9e.+-]+)  # allow -1.234e+56
				$
				}x) {
		print STDERR "cannot parse as ETV: \"".$+{vecdata}."\"\n";
		next;
	}

	$id_to_firstTime{$vecnum} = (!defined($id_to_firstTime{$vecnum})) ? $+{startTime} : min($id_to_firstTime{$vecnum}, $+{startTime});
	$id_to_lastTime{$vecnum} = (!defined($id_to_lastTime{$vecnum})) ? $+{endTime} : max($id_to_lastTime{$vecnum}, $+{endTime});
	$id_to_count{$vecnum} = (!defined($id_to_count{$vecnum})) ? $+{count} : ($id_to_count{$vecnum} + $+{count});
	$id_to_min{$vecnum} = (!defined($id_to_min{$vecnum})) ? $+{min} : min($id_to_min{$vecnum}, $+{min});
	$id_to_max{$vecnum} = (!defined($id_to_max{$vecnum})) ? $+{max} : max($id_to_max{$vecnum}, $+{max});
	$id_to_sum{$vecnum} = (!defined($id_to_sum{$vecnum})) ? $+{sum} : ($id_to_sum{$vecnum} + $+{sum});
	$id_to_sumSqr{$vecnum} = (!defined($id_to_sumSqr{$vecnum})) ? $+{sumSqr} : ($id_to_sumSqr{$vecnum} + $+{sumSqr});

} 

#print "configname"."\t"."runnumber"."\t"."repetition"."\t".
print "module"."\t"."name"."\t"."firstTime"."\t"."lastTime"."\t"."count"."\t"."min"."\t"."max"."\t"."sum"."\t"."sumSqr"."\n";

foreach my $vecnum (keys %id_to_node) {
	print $id_to_node{$vecnum} . "\t";
	print $id_to_name{$vecnum} . "\t";
	print $id_to_firstTime{$vecnum} . "\t";
	print $id_to_lastTime{$vecnum} . "\t";
	print $id_to_count{$vecnum} . "\t";
	print $id_to_min{$vecnum} . "\t";
	print $id_to_max{$vecnum} . "\t";
	print $id_to_sum{$vecnum} . "\t";
	print $id_to_sumSqr{$vecnum};
	print "\n";
}
