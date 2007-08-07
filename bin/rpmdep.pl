#!/usr/bin/perl
#
#	rpmdep.pl
#	Generate Graphviz tree from RPM dependencies
#	Copyright © Jan Engelhardt <jengelh [at] gmx de>, 2005 - 2007
#
#	This program is free software; you can redistribute it and/or modify
#	it under the terms of the GNU General Public License as published by
#	the Free Software Foundation; however ONLY version 2 of the License.
#	For details, see the file named "LICENSE.GPL2".
#
# Run `rpmdep.pl --help`.

use Data::Dumper;
use Getopt::Long;
use strict;
our $FOLEVEL = 4;
&main();

sub main()
{
	local *FH;
	my($start_file, $start_level) = (undef, 1);
	my($stage1_out, $stage2_out, $stage3_out) = (undef, undef, undef);
	my $stage4_out = "-";
	my $w;

	&GetOptions(
		"fanout-height|fanout-width=i" => \$FOLEVEL,
		"start-file=s"  => \$start_file,
		"start-level=i" => \$start_level,
		"stage1-out=s"  => \$stage1_out,
		"stage2-out=s"  => \$stage2_out,
		"stage3-out=s"  => \$stage3_out,
		"stage4-out=s"  => \$stage4_out,
		"h|help"        => \&help,
	);
	if ($start_file) {
		$w = do $start_file;
	}
	if ($start_level == 1) {
		$w = &stage1($stage1_out);
		++$start_level;
	}
	if ($start_level == 2) {
		&stage2($stage2_out, $w);
		++$start_level;
	}
	if ($start_level == 3) {
		&stage3($stage3_out, $w);
		++$start_level;
	}
	if ($start_level == 4) {
		&printout($stage4_out, $w);
	}
	return;
}

sub strip_vra($)
{
	my $pkg = shift @_;
	$pkg =~ s/-[\w\.]+-[\w\.]+\.\w+$//; # version-release-arch
	return $pkg
}

sub stage1($)
{
	print STDERR "# Stage 1: Finding dependencies\n";

	my $stage1_out = shift @_;
	my @pkglist    = `rpm -qa --qf='\%{NAME}\\n'`;
	my $pkghash    = {};

	for (my $count = 0; $count <= $#pkglist; ++$count) {
		my $pkg = $pkglist[$count];
		my %h;

		chomp $pkg;
		printf STDERR "#\t(%u/%u) $pkg\n", $count + 1, $#pkglist + 1;

		foreach my $src (`rpm --test -e "$pkg" 2>&1`) {
			if (substr($src, 0, 1) ne "\t") {
				next;
			}
			chomp $src;
			my($ess) = ($src =~ / is needed by \(installed\) (.*)/);
			$h{&strip_vra($ess)} = 1;
		}

		%{$pkghash->{$pkg}} = %h;
	}

	if ($stage1_out ne "") {
		local *FH;
		open(FH, "> $stage1_out") ||
			warn "Could not open $stage1_out: $!\n";
		print FH Dumper($pkghash);
		close FH;
	}

	return $pkghash;
}

sub count_deps($)
{
	my $pkghash = shift @_;
	my $ret = 0;

	foreach my $pkg (keys %$pkghash) {
		$ret += scalar keys %{$pkghash->{$pkg}};
	}

	return $ret;
}

sub stage2($$)
{
	print STDERR "# Stage 2: Reduce visible links\n";

	my $stage2_out = shift @_;
	my $pkghash    = shift @_;
	my $prevtotal;

	print STDERR "#\tinput: ", &count_deps($pkghash), " dependencies\n";
	foreach my $firstpkg (keys %$pkghash) {
		my $href = $pkghash->{$firstpkg};
		foreach my $firstdep (keys %$href) {
			foreach my $scndpkg (keys %$pkghash) {
				if ($firstpkg eq $scndpkg) {
					next;
				}
				if ($pkghash->{$scndpkg}->{$firstdep}) {
#					print STDERR "#\t$firstdep->$firstpkg->$scndpkg, DELETE $firstdep->$scndpkg\n";
					delete $pkghash->{$scndpkg}->{$firstdep};
				}
			}
		}
	}

	print STDERR "#\toutput: ", &count_deps($pkghash), " dependencies\n";

	if ($stage2_out ne "") {
		local *FH;
		open(FH, "> $stage2_out") ||
			warn "Could not open $stage2_out: $!\n";
		print FH Dumper($pkghash);
		close FH;
	}

	return $pkghash;
}

sub alphanumeric($$)
{
	my($a_alpha, $a_num) = ($_[0] =~ /^(.*?)(?:<(\d+)>)?$/);
	my($b_alpha, $b_num) = ($_[1] =~ /^(.*?)(?:<(\d+)>)?$/);
	return $a_alpha cmp $b_alpha || $a_num <=> $b_num;
}

sub stage3($$)
{
	print STDERR "# Stage 3: Link simplexer\n";

	my $stage3_out = shift @_;
	my $pkghash    = shift @_;
	my $counter    = 1;
	my $countmax   = scalar keys %$pkghash;
	my $newhash    = {};

	foreach my $pkg (keys %$pkghash) {
		my $href  = $pkghash->{$pkg};
		my $iter  = 1;
		my $reloc = 0;

		for (my $elem; ($elem = scalar keys %$href) > $FOLEVEL;
		    ++$iter)
		{
			my $nphash = {};

			printf STDERR "#\t(%u/%u) %s round %u, height %u\n",
			              $counter, $countmax, $pkg, $iter, $elem;

			$reloc += $FOLEVEL - 1;
			foreach my $ess (sort alphanumeric keys %$href) {
				my $newpkg = "$pkg<".int($reloc++ / $FOLEVEL).">";
				$nphash->{$newpkg}->{$ess} = 1;
				delete $href->{$ess};
#				print STDERR "#\t$ess => $newpkg\n";
			}
			foreach my $ess (keys %$nphash) {
				$href->{$ess} = 1;
			}
			&merge_hash($newhash, $nphash);
		}
		++$counter;
	}

	&merge_hash($pkghash, $newhash);

	if ($stage3_out ne "") {
		local *FH;
		open(FH, "> $stage3_out") ||
			warn "Could not open $stage3_out: $!\n";
		print FH Dumper($pkghash);
		close FH;
	}

	return;
}

sub merge_hash($@)
{
	my $target = shift @_;
	foreach my $source (@_) {
		foreach my $key (keys %$source) {
			$target->{$key} = $source->{$key};
		}
	}
	return;
}

sub printout($)
{
	my $stage4_out = shift @_;
	my $pkghash    = shift @_;
	local *FH;

	open(FH, "> $stage4_out") || warn "Could not open $stage4_out: $!\n";

	print FH "digraph deps {\n";
	print FH "\trankdir=LR;\n";
	foreach my $pkg (sort alphanumeric keys %$pkghash) {
		my $href = $pkghash->{$pkg};
		if ($pkg =~ /<\d+>$/) {
			# routing node
			print FH "\t\"$pkg\" [shape=\"point\"];\n";
		}
		foreach my $ess (sort alphanumeric keys %$href) {
			print FH "\t\"$ess\" -> \"$pkg\" [arrowhead=\"none\"];\n";
		}
	}
	print FH "}\n";
	return;
}

sub help()
{
	print <<"--EOF";

Syntax: $0 [options]

	--fanout-height=N       have at most N packages per level
	--start-file=FILE       source FILE for internal data
	--start-level=N         start at stage N
	--stage1-out=FILE	save post-stage1 data to FILE
	--stage2-out=FILE	save post-stage2 data to FILE
	--stage3-out=FILE	save post-stage3 data to FILE
	--stage4-out=FILE	output Graphviz data to FILE (default: stdout)

Common use (requires graphviz):
	rpmdep.pl | dot -T svg >output.svg

Example for fast regeneration of graph:
	rpmdep.pl --stage3-out=foo --fanout-height=32 | dot ...
	rpmdep.pl --start-file=foo --start-level=4 --fanout-height=2 | dot ...

--EOF
	exit 0;
}
