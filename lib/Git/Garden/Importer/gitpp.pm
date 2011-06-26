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
use File::Spec;

use lib '../../..';
use Git::Garden::Commit;

sub prepare_commits {
    my ( $self, $dir ) = @_;

    my ( $refs, $commits ) = get_git_meta_data( $dir );

    my @garden_commits = map convert_to_garden_commit( $commits->[$_], $_, $refs ), 0 .. $#{$commits};

    return \@garden_commits;
}

sub convert_to_garden_commit {
    my ( $commit, $sort_index, $refs ) = @_;

    return Git::Garden::Commit->new(
        uid         => $commit->sha1,
        sort_index  => $sort_index,
        comment     => $commit->comment,
        parent_uids => $commit->parent_sha1s,
        labels      => $refs->{ $commit->sha1 } || [],
    );
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

    my $refs = extract_ref_commits( $git );

    @commits = sort_by_sha1( \@commits );

    return ( $refs, \@commits );
}

sub sort_by_sha1 {
    my ( $commits ) = @_;

    my @commits = sort { $b->committed_time->datetime cmp $a->committed_time->datetime } @{$commits};
    my %commits_by_sha = map { $_->sha1 => $_ } @commits;

    my @sorted_commits;
    my $last_commit_count = 0;

    while ( values %commits_by_sha ) {
        for my $commit ( @commits ) {
            my $sha1 = $commit->sha1;
            next if !$commits_by_sha{$sha1};

            my @child_commits = grep is_child_of( $sha1, $_ ), values %commits_by_sha;
            next if @child_commits;

            push @sorted_commits, $commit;
            delete $commits_by_sha{$sha1};
            last;
        }
        die "commit-sorting got stuck" if @sorted_commits eq $last_commit_count;
        $last_commit_count = @sorted_commits;
    }

    return @sorted_commits;
}

sub is_child_of {
    my ( $sha1, $commit ) = @_;
    return 1 if grep { $sha1 eq $_ } @{ $commit->parent_sha1s };
    return;
}

sub extract_ref_commits {
    my ( $git ) = @_;

    my %ref_names = map { $_ => 1 } $git->ref_names;
    my %refs;
    push @{ $refs{ $git->ref_sha1( $_ ) } }, $_ for keys %ref_names;

    return \%refs;
}

sub get_real_git_dir {
    my ( $dir ) = @_;

    return $dir           if -d $dir . "/refs/heads";
    return $dir . "/.git" if -d $dir . "/.git/refs/heads";
    return $dir . "/git"  if -d $dir . "/git/refs/heads";
    die "could not identify git meta-data directory '$dir', expecting a path to something that contains the dir refs/heads";
}

1;
