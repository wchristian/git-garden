#!/usr/bin/perl

use strict;

sub getcf($$)
{
	local *FH = shift @_;
	my $size  = shift @_;
	my $buf;
	read(FH, $buf, $size);
	return $buf;
}

sub mkdir_p($)
{
	my $fullpath = shift @_;
	my @list;

	$fullpath =~ s/\/+$//go;
	$fullpath =~ s/^~(?!\w)/$ENV{HOME}/so;
	$fullpath =~ s/\/$//so;
	@list = split(/\//so, $fullpath);

	for (my $i = 0; $i < scalar(@list); ++$i) {
		my $dir = join("/", @list[0..$i]);
		if ($dir eq "") {
			next;
		}
		if (!-e $dir && !mkdir($dir)) {
			return 0;
                }
	}

	return 1;
}

sub transfer($$$$)
{
	local *OUT = shift @_;
	local *IN  = shift @_;
	my $size   = shift @_;
	my $offset = shift @_;
	my $saved_offset;
	my $buf;

	if (defined($offset)) {
		$saved_offset = tell(IN);
		seek(IN, $offset, 0);
	}

	while ($size > 0) {
		my $rem = ($size > 4096) ? 4096 : $size;
		read(IN, $buf, $rem);
		print OUT $buf;
		$size -= $rem;
	}

	if (defined($offset)) {
		seek(IN, $saved_offset, 0);
	}

	return;
}

1;
