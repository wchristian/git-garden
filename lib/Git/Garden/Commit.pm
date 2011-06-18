use strict;
use warnings;

package Git::Garden::Commit;

use Moo;

has uid          => ( is => 'ro', required => 1 );
has sort_index   => ( is => 'ro', required => 1 );
has parent_uids  => ( is => 'ro', required => 1 );
has labels       => ( is => 'ro', required => 1 );
has parent_count => ( is => 'ro', lazy     => 1, builder => '_build_parent_count' );
has comment => ( is => 'ro' );

sub _build_parent_count {
    my ( $self ) = @_;
    return scalar @{ $self->parent_uids };
}

sub sorted_parents {
    my ( $self, $commits_by_uid ) = @_;

    my @parents = grep $_, map $commits_by_uid->{$_}, @{ $self->parent_uids };

    return @parents if @parents < 2;

    @parents = sort { $a->{sort_index} <=> $b->{sort_index} } @parents;
    @parents = sort { $a->merge_depth( $commits_by_uid ) <=> $b->merge_depth( $commits_by_uid ) } @parents;

    return @parents;
}

sub merge_depth {
    my ( $commit, $commits_by_uid ) = @_;

    my $depth = 0;

    while ( $commit ) {
        last if !$commit->parent_count;
        return $depth if $commit->parent_count > 1;

        $depth++;
        $commit = $commits_by_uid->{ $commit->parent_uids->[0] };
    }

    return scalar keys %{$commits_by_uid};
}

1;
