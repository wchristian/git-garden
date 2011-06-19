use strict;
use warnings;

package Git::Garden::Importer::fossil;

# ABSTRACT: import fossil repo data as a list of commits with refs and other metadata

=head1 SYNOPSIS

    use Git::Garden::Importer::fossil;
    my $commit_array_ref = Git::Garden::Importer::fossil->prepare_commits( $fossil_file );

=cut

use DBIx::Simple;

use lib '../../..';
use Git::Garden::Commit;

sub prepare_commits {
    my ( $self, $target ) = @_;

    my ( $refs, $commits ) = get_git_meta_data( $target );

    my @garden_commits = map convert_to_garden_commit( $commits->[$_], $_, $refs ), 0 .. $#{$commits};

    return \@garden_commits;
}

sub convert_to_garden_commit {
    my ( $commit, $sort_index, $refs ) = @_;

    return Git::Garden::Commit->new(
        uid         => $commit->{objid},
        sort_index  => $sort_index,
        parent_uids => $commit->{parents} || [],
        labels      => $refs->{ $commit->{objid} } || [],
        comment     => $commit->{comment},
    );
}

sub get_git_meta_data {
    my ( $target ) = @_;

    my $db = DBIx::Simple->new( "dbi:SQLite:dbname=$target", "", "" );

    my @commits = $db->query( "select * from event where type = 'ci' order by mtime desc" )->hashes;

    my %commits = map { $_->{objid} => $_ } @commits;

    my @parent_links = $db->query( "select * from plink" )->hashes;
    push @{ $commits{ $_->{cid} }{parents} }, $_->{pid} for @parent_links;

    my @tags = $db->query( "select *, l.rid as head_id from tag t left join tagxref tx on t.tagid = tx.tagid left join leaf l ON l.rid = tx.rid order by rid" )->hashes;

    my %refs;
    push @{ $refs{$_->{head_id}}}, $_->{value} for grep { $_->{tagname} eq 'branch' and $_->{head_id} } @tags;

    return ( \%refs, \@commits );
}

1;
