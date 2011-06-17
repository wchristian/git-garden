use strict;
use warnings;

package Git::Garden::Render::graphviz;

# ABSTRACT: plot a Git::Garden grid with GraphViz

=head1 SYNOPSIS

    use Git::Garden::Render::graphviz;
    my $png = Git::Garden::Render::graphviz->plot_grid( $grid );

=cut

use GraphViz;

sub plot_grid {
    my ( $self, $grid ) = @_;

    my $g = GraphViz->new( edge => { dir => 'back' } );

    $g->add_node( $_->{commit}{mini_sha} ) for @{$grid};

    for my $row ( @{$grid} ) {
        $g->add_edge( $row->{commit}{mini_sha} => $_->{mini_sha} ) for @{ $row->{commit}{parents} };
    }

    return $g->as_png;
}

1;
