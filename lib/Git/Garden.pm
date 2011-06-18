use strict;
use warnings;

package Git::Garden;

# ABSTRACT: create a git graph grid from a list of commits with metadata

=head1 SYNOPSIS

    use Git::Garden 'create_git_graph_grid';
    my $git_graph_grid = create_git_graph_grid( $commit_array_ref );

=cut

use Sub::Exporter::Simple 'create_git_graph_grid';

use List::Util 'max';

sub create_git_graph_grid {
    my ( $commits ) = @_;

    my %commits_by_uid = map { $_->uid => $_ } @{$commits};

    my @grid;
    for my $commit ( @{$commits} ) {
        my $prev_row = $grid[$#grid] || { columns => [] };
        my $row = { index => $#grid + 1, commit => $commit, columns => [], parents_to_match => $commit->parent_count };
        push @grid, $row;

        mark_commit_column( $row, $prev_row, $commit );

        mark_expecting_column( $row, $prev_row, $commit, $_, \%commits_by_uid ) for $commit->sorted_parents( \%commits_by_uid );

        mark_expectations_and_branch_points( $prev_row, $row );
    }

    return \@grid;
}

sub mark_commit_column {
    my ( $row, $prev_row, $commit ) = @_;

    $row->{commit_column_index} = find_commit_column_index( $prev_row, $row, $commit );
    $commit->parent_count;
    add_visual_to_column( $row, $row->{commit_column_index}, 'commit' );
    add_visual_to_column( $row, $row->{commit_column_index}, 'merge' ) if $commit->parent_count > 1;

    return;
}

sub mark_expecting_column {
    my ( $row, $prev_row, $commit, $parent, $commits_by_uid ) = @_;

    my $expecting_column_index = find_column_index_for_expected_parent( $row, $prev_row, $parent, $commits_by_uid );
    add_visual_to_column( $row, $expecting_column_index, 'merge_point' ) if $expecting_column_index != $row->{commit_column_index};
    $row->{columns}[$expecting_column_index]{expected_uid} = $parent->{uid};

    $row->{parents_to_match}--;

    return;
}

sub mark_expectations_and_branch_points {
    my ( $prev_row, $row ) = @_;

    my $columns      = $row->{columns};
    my $prev_columns = $prev_row->{columns};

    for my $prev_col ( @{$prev_columns} ) {
        next if !$prev_col;
        next if !$prev_col->{expected_uid};

        if ( $prev_col->{expected_uid} ne $row->{commit}{uid} ) {
            add_visual_to_column( $row, $prev_col->{index}, "expects_commit" );
            $row->{columns}[ $prev_col->{index} ]{expected_uid} = $prev_col->{expected_uid};
        }
        elsif ( $row->{commit_column_index} != $prev_col->{index} ) {
            add_visual_to_column( $row, $prev_col->{index}, "branch_point" );
        }
    }

    return;
}

sub find_commit_column_index {
    my ( $prev_row, $row, $commit ) = @_;

    my $column_index = find_expected_column_index_for_commit( $prev_row, $row->{commit} );
    $column_index //= find_next_empty_column_index( $prev_row );

    return $column_index;
}

sub add_visual_to_column {
    my ( $row, $column_index, $visual ) = @_;

    my $commit_col = $row->{columns}[$column_index] ||= { index => $column_index };
    $commit_col->{visuals}{$visual} = 1;

    return;
}

sub find_column_index_for_expected_parent {
    my ( $row, $prev_row, $parent, $commits_by_uid ) = @_;

    my $columns                         = $row->{columns};
    my $commit_column_index             = $row->{commit_column_index};
    my $parents_with_higher_merge_depth = parents_with_higher_merge_depth( $row, $parent, $commits_by_uid );

    my @expecting_cols = grep col_expects_this_parent( $_, $parent ), @{ $prev_row->{columns} };
    return $expecting_cols[0]{index} if @expecting_cols and $row->{parents_to_match} > 1 and $expecting_cols[0]{index} >= $commit_column_index and !$parents_with_higher_merge_depth;

    return $commit_column_index if $columns->[$commit_column_index] and !$columns->[$commit_column_index]{expected_uid} and ( !@{ $parent->{labels} } or $row->{parents_to_match} == 1 );

    return $expecting_cols[0]{index} if @expecting_cols and $expecting_cols[0]{index} >= $commit_column_index;

    my $prev_cols = $prev_row->{columns};
    my $max_look = max( $#{$columns}, $#{$prev_cols} );

    for my $column_index ( 0 .. $max_look ) {
        my $column = $columns->[$column_index];

        next if $prev_cols->[$column_index] and $prev_cols->[$column_index]{expected_uid} and $prev_cols->[$column_index]{expected_uid} ne $parent->{uid};
        next if $column_index <= $commit_column_index;

        return $column_index if !$column;
        return $column_index if !$column->{expected_uid};
    }

    return $max_look + 1;
}

sub parents_with_higher_merge_depth {
    my ( $row, $parent, $commits_by_uid ) = @_;

    $DB::single = 1 if $parent->sort_index == 18;

    my @parents = $row->{commit}->sorted_parents( $commits_by_uid );
    @parents = grep { $_->merge_depth( $commits_by_uid ) > $parent->merge_depth( $commits_by_uid ) } @parents;

    return scalar @parents;
}

sub find_expected_column_index_for_commit {
    my ( $prev_row, $commit ) = @_;

    for my $column ( @{ $prev_row->{columns} } ) {
        next if !$column;
        next if !$column->{expected_uid};
        next if $column->{expected_uid} ne $commit->{uid};

        return $column->{index};
    }

    return;
}

sub find_next_empty_column_index {
    my ( $prev_row ) = @_;

    my $columns = $prev_row->{columns};
    for my $column_index ( 0 .. $#{$columns} ) {
        next if $columns->[$column_index];
        return $column_index;
    }

    return $#{$columns} + 1;
}

sub col_expects_this_parent {
    my ( $col, $parent ) = @_;

    return if !$col;
    return if !$col->{expected_uid};
    return if $col->{expected_uid} ne $parent->{uid};

    return 1;
}

1;
