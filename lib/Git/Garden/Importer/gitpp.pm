use strict;
use warnings;

package Git::Garden::Importer::gitpp;

# ABSTRACT: import git repo data as a list of commits with refs and other metadata

=head1 SYNOPSIS

    use Git::Garden::Importer::gitpp;
    my $commit_array_ref = Git::Garden::Importer::gitpp->prepare_commits( $git_directory );

=cut

use lib 'd:/git-pureperl/lib';
use Git::PurePerl;
use Git::Class::Cmd;
use File::Spec;

sub git { Git::Class::Cmd->new }

sub prepare_commits {
    my ( $self, $dir ) = @_;

    my ( $refs, $commits, $commit_count ) = get_git_meta_data( $dir );

    my %commits = map { $_->sha1, $_ } @{$commits};
    for my $commit ( @{$commits} ) {
        my @parents = map $commits{$_}, @{ $commit->parent_sha1s };
        @parents = sort { $b->committed_time <=> $a->committed_time } @parents;
        $commit->{uid}         = $commit->sha1;
        $commit->{parents}     = \@parents;
        $commit->{labels}      = $refs->{ $commit->sha1 } || [];
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

    my @dirs = File::Spec->splitdir( $real_dir );
    pop @dirs;
    my $root_dir = File::Spec->catdir( @dirs );
    my $git = Git::PurePerl->new( directory => $root_dir, gitdir => $real_dir );

    my $commit_stream = $git->all_objects;
    my @objects;
    while ( my $object_block = $commit_stream->next ) {
        push @objects, @{$object_block};
    }
    my @commits = grep { $_->kind eq 'commit' } @objects;
    @commits = sort { $b->committed_time <=> $a->committed_time } @commits;

    my %ref_names = map { $_ => 1 } $git->ref_names;
    my %refs;
    push @{ $refs{ $git->ref_sha1( $_ ) } }, $_ for keys %ref_names;

    return ( \%refs, \@commits, scalar @commits );
}

sub get_real_git_dir {
    my ( $dir ) = @_;

    return $dir           if -d $dir . "/refs/heads";
    return $dir . "/.git" if -d $dir . "/.git/refs/heads";
    return $dir . "/git"  if -d $dir . "/git/refs/heads";
    die "could not identify git meta-data directory '$dir', expecting a path to something that contains the dir refs/heads";
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
