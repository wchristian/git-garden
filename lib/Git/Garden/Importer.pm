use strict;
use warnings;

package Git::Garden::Importer;

# ABSTRACT: import git repo data as a list of commits with refs and other metadata

=head1 SYNOPSIS

    use Git::Garden::Importer 'prepare_commits';
    my $commit_array_ref = prepare_commits( $git_directory );

=cut

use Sub::Exporter::Simple 'prepare_commits';

use Git::Class::Cmd;

sub git { Git::Class::Cmd->new }

sub prepare_commits {
    my ( $dir ) = @_;

    my ( $refs, $commits, $commit_count ) = get_git_meta_data( $dir );

    $commits->[$_]->{index} = $_ for 0 .. $#{$commits};

    my %commits = map { $_->{sha} => $_ } @{$commits};
    for my $commit ( @{$commits} ) {
        my @parents = map $commits{$_}, @{ $commit->{parents} };
        @parents = sort { $a->{index} <=> $b->{index} } @parents;
        $commit->{parents}     = \@parents;
        $commit->{refs}        = $refs->{ $commit->{sha} } || [];
        $commit->{merge_depth} = -1;
    }

    for my $commit ( @{$commits} ) {
        my @parents = @{ $commit->{parents} };
        next if @parents < 2;

        $_->{merge_depth} = find_merge_depth( $_, $commit_count ) for @parents;
        @parents = sort { $a->{merge_depth} <=> $b->{merge_depth} } @parents;
        $commit->{parents} = \@parents;
    }

    return $commits;
}

sub get_git_meta_data {
    my ( $dir ) = @_;

    my $real_dir = get_real_git_dir( $dir );

    $ENV{GIT_DIR} = $real_dir;

    my $refs = get_refs();

    my @commits = map parse_commit( $_ ), git()->log( "--date-order", "--pretty=format:{%H}{%h}{%P}%s", "--all", "HEAD" );

    delete $ENV{GIT_DIR};

    return ( $refs, \@commits, scalar @commits );
}

sub get_real_git_dir {
    my ( $dir ) = @_;

    return $dir           if -d $dir . "/refs/heads";
    return $dir . "/.git" if -d $dir . "/.git/refs/heads";
    return $dir . "/git"  if -d $dir . "/git/refs/heads";
    die "could not identify git meta-data directory, expecting a path to something that contains the dir refs/heads";
}

sub get_refs {
    my @commits = git()->git( "show-ref", "-d" );

    my %refs;

    for my $ln ( @commits ) {
        next if !$ln;

        my ( $sha, $name ) = ( $ln =~ /^(\S+)\s+(.*)/s );
        $name =~ s/\^\{\}//;
        $refs{$sha} = [] if !exists $refs{$sha};
        push @{ $refs{$sha} }, $name;
    }

    my $head = git()->git( "rev-parse", "HEAD" );
    chomp $head;
    unshift @{ $refs{$head} }, "HEAD";

    return \%refs;
}

sub parse_commit {
    my ( $line ) = @_;

    my ( $sha, $mini_sha, $parents, $msg ) = ( $line =~ /^\{(.*?)\}\{(.*?)\}\{(.*?)\}(.*)/s );

    my %commit = (
        sha      => $sha,
        mini_sha => $mini_sha,
        parents  => [ split " ", $parents ],
        msg      => $msg
    );

    return \%commit;
}

sub find_merge_depth {
    my ( $commit, $commit_count ) = @_;

    my $depth = 0;

    while ( $commit ) {
        return $depth if @{ $commit->{parents} } > 1;

        $depth++;
        $commit = $commit->{parents}[0];
    }

    return $commit_count;
}

1;
