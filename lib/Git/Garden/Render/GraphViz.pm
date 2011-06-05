use strict;
use warnings;

package Git::Garden::Render::GraphViz;

# ABSTRACT: plot a Git::Garden grid with GraphViz

=head1 SYNOPSIS

    use Git::Garden::Render::GraphViz 'graphvizplot_grid';
    graphvizplot_grid( $grid );

    firefox git.png

=cut

use Sub::Exporter::Simple 'graphvizplot_grid';

use GraphViz;
use File::Slurp 'write_file';

sub graphvizplot_grid {
    my ( $grid ) = @_;

    my $g = GraphViz->new( edge => { dir => 'back' } );

    $g->add_node( $_->{commit}{mini_sha} ) for @{$grid};

    for my $row ( @{$grid} ) {
        $g->add_edge( $row->{commit}{mini_sha} => $_->{mini_sha} ) for @{ $row->{commit}{parents} };
    }

    write_file 'git.png', { binmode => ':raw' }, $g->as_png;

    return;
}

1;
