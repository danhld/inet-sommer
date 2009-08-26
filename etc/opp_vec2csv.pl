#!/usr/bin/perl -Tw

#
# vec2csv_multi.pl - Outputs OMNeT++ 4 output vector files in CSV format, collating values from multiple vectors into one column each
#
# Copyright (C) 2008 Christoph Sommer <christoph.sommer@informatik.uni-erlangen.de>
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

my $splitDups = 1;
my $discardDups = 0;
my $addDups = 0;
GetOptions (
	"split-dups|s" => \$splitDups,
	"discard-dups|d" => \$discardDups,
	"add-dups|a" => \$addDups
);
$splitDups = 0 if ($addDups || $discardDups);
$addDups = 0 if ($discardDups);

if (@ARGV < 1) {
	print STDERR "usage: vec2csv_multi.pl [-s|-a|-d] <vec_name> [<vec_name> ...] \n";
	print STDERR "\n";
	print STDERR "  -s --split-dups:   Of multiple observations made at the same time\n";
	print STDERR "                     multiple records will be created\n";
	print STDERR "                     (this is the default)\n";
	print STDERR "\n";
	print STDERR "  -a --add-dups:     Of multiple observations made at the same time\n";
	print STDERR "                     the sum of all observations will be recorded\n";
	print STDERR "\n";
	print STDERR "  -d --discard-dups: Of multiple observations made at the same time\n";
	print STDERR "                     only the last observation will be recorded\n";
	print STDERR "\n";
	print STDERR "e.g.: vec2csv_multi.pl receivedBytes senderName <input.vec >output.csv\n";
	exit;
}

my @vec_names;
my %vec_known;
while (my $vec_name = shift @ARGV) {
	push (@vec_names, $vec_name);
	$vec_known{$vec_name} = 1;
}

print "nod_name"."\t"."time";
foreach my $vec_name (@vec_names) {
	print "\t".$vec_name;
}
print "\n";

my @nodName; # vector_id <-> nod_name mappings
my @vecName; # vector_id <-> vec_name mappings
my @vecType; # vector_id <-> type mappings
my $current_nod_name = "";
my $current_time = "";
my %vec_values = ();
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
		$nodName[$+{vecnum}] = defined($+{nodname1})?$+{nodname1}:"" . defined($+{nodname2})?$+{nodname2}:"";
		$vecName[$+{vecnum}] = defined($+{vecname1})?$+{vecname1}:"" . defined($+{vecname2})?$+{vecname2}:"";
		$vecType[$+{vecnum}] = defined($+{vectype})?$+{vectype}:"";
		next;
	}

	# line must contain vector data
	next unless (m{^(?<vecnum>[0-9]+)\s+(?<vecdata>.*)\r?\n$}x);

	my $nod_name = $nodName[$+{vecnum}];
	my $vec_name = $vecName[$+{vecnum}];
	my $vec_type = $vecType[$+{vecnum}];

	# vec_name must be among those given on cmdline
	next unless exists($vec_known{$vec_name});

	# extract time and value
	my $time = "";
	my $value = "";
	if ($vec_type eq 'ETV') {
		unless ($+{vecdata} =~ m{
					^
					(?<event>[0-9]+)
					\s+
					(?<time>[0-9e.+-]+)  # allow -1.234e+56
					\s+
					(?<value>[0-9e.+-]+)  # allow -1.234e+56
					$
					}x) {
			print STDERR "cannot parse as ETV: \"".$+{vecdata}."\"\n";
			next;
		}
		$time = $+{time};
		$value = $+{value};
	} else {
		print STDERR "unknown vector type: \"".$vec_type."\"\n";
		next;
	}

	# new nod_name or time?
	if ((!($nod_name eq $current_nod_name)) || ($time ne $current_time) || ($splitDups && exists $vec_values{$vec_name})) {

		# see if there's anything in the buffer, print it
		if (scalar(%vec_values)) {
			print $current_nod_name."\t".$current_time;
			foreach my $vec_name (@vec_names) {
				my $value = $vec_values{$vec_name};
				print "\t".(defined $value ? $value : "");
			}
			print "\n";
		}

		# start over
		$current_nod_name = $nod_name;
		$current_time = $time;
		%vec_values = ();

	}

	# buffer value
	if ($addDups) {
		$vec_values{$vec_name} += $value;
	} else {
		$vec_values{$vec_name} = $value;
	}

} 

# see if there's anything in the buffer, print it
if (scalar(%vec_values)) {
	print $current_nod_name."\t".$current_time;
	foreach my $vec_name (@vec_names) {
		my $value = $vec_values{$vec_name};
		print "\t".(defined $value ? $value : "");
	}
	print "\n";
}

