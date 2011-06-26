use strict;
use warnings;

package Git::Garden::Importer::gitclass;

# ABSTRACT: import git repo data as a list of commits with refs and other metadata

=head1 SYNOPSIS

    use Git::Garden::Importer::gitclass;
    my $commit_array_ref = Git::Garden::Importer::gitclass->prepare_commits( $git_directory );

=cut

use Git::Class::Cmd;
use File::Spec;

use lib '../../..';
use Git::Garden::Commit;

sub git { Git::Class::Cmd->new }

sub prepare_commits {
    my ( $self, $dir ) = @_;

    my ( $refs, $commits ) = get_git_meta_data( $dir );

    my @garden_commits = map convert_to_garden_commit( $commits->[$_], $_, $refs ), 0 .. $#{$commits};

    return \@garden_commits;
}

sub convert_to_garden_commit {
    my ( $commit, $sort_index, $refs ) = @_;

    return Git::Garden::Commit->new(
        uid         => $commit->{uid},
        sort_index  => $sort_index,
        comment     => $commit->{comment},
        parent_uids => $commit->{parents},
        labels      => $refs->{ $commit->{uid} } || [],
    );
}

sub get_git_meta_data {
    my ( $dir ) = @_;

    my $real_dir = get_real_git_dir( $dir );

    $ENV{GIT_DIR} = $real_dir;

    my $refs = get_refs();

    my @commits = map parse_commit( $_ ), git()->log( "--date-order", "--pretty=format:{%H}{%h}{%P}%s", "--all", "HEAD" );

    delete $ENV{GIT_DIR};

    return ( $refs, \@commits );
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

    my ( $sha, $mini_sha, $parents, $comment ) = ( $line =~ /^\{(.*?)\}\{(.*?)\}\{(.*?)\}(.*)/s );

    my %commit = (
        uid      => $sha,
        mini_sha => $mini_sha,
        parents  => [ split " ", $parents ],
        comment  => $comment
    );

    return \%commit;
}

1;
