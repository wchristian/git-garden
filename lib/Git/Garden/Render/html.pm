use strict;
use warnings;

package Git::Garden::Render::html;

# ABSTRACT: plot a Git::Garden grid as a single HTML file

=head1 SYNOPSIS

    use Git::Garden::Render::html;
    my $html = Git::Garden::Render::html->plot_grid( $grid );

=cut

use JSON 'to_json';

sub html_row {
    my ( $row )  = @_;

    return if !$row;

    my $commit = $row->{commit};
    my $uid = $commit->{uid};
    my $comment = $commit->{comment};
    my $labels = join '', map "<b>[$_]</b>", sort @{$commit->{labels}};
    $labels .= " " if $labels;

    return qq|
        <tr>
            <td id="graph_$uid"></td>
            <td>$uid</td>
            <td class="shorten">$labels$comment</td>
        </tr>
    |;
}

sub plot_grid {
    my ( $self, $grid ) = @_;

    my $rows = join "\n", map html_row( $_ ), @{$grid};

    my $json = to_json( $grid, { allow_blessed => 1, convert_blessed => 1 } );

    my $js = graphlog_js();

    my $html = qq|
<!DOCTYPE html>

<html>
<head>
    <script type="text/javascript" src="http://code.jquery.com/jquery-latest.min.js"></script>
    <script type="text/javascript" src="http://plugins.learningjquery.com/expander/jquery.expander.js"></script>
    <script type="text/javascript" src="http://github.com/DmitryBaranovskiy/raphael/raw/master/raphael.js"></script>
    <script type="text/javascript">
        <!--

            \$(document).ready(
                function() {
                    \$('td.shorten').expander(
                        {
                            slicePoint: 100,
                            widow: 2,
                            expandEffect: 'show',
                            userCollapseText: '[^]',
                            expandText:       'more...',
                            afterExpand: function( elem ) {
                                var siblings = elem.siblings();
                                var graph_container = siblings.first();
                                var children = graph_container.children();
                                //children.first().remove();
                                return;
                            },
                            onCollapse: function( elem, byUser ) {
                                return;
                            }
                        }
                    );
                }
            );

            $js

            commit_rows = jQuery.makeArray( $json );

            \$(document).ready(function() {
                do_graphs( commit_rows );
            });

        -->
    </script>
</head>

<body>
    <table style="border-collapse:collapse; border-spacing: 0px;">
        <tr>
            <th style="width:200px;">Graph</td>
            <th>Unique ID</th>
            <th>Comment</th>
        </tr>
        $rows
    </table>
</body>
    |;

    return $html;
}

sub graphlog_js {
q@
function do_graphs( commit_rows ) {

    var colors = fill_colors();
    var max_column = get_max_column( commit_rows );

    $(commit_rows).each(function () {
        this.canvas = render_canvas( 'graph_' + this.commit.uid );
        return;
    });

    var min_size_limit = get_min_size_limit( commit_rows, max_column );

    $(commit_rows).each(function () {
        render_commit_row ( this, commit_rows, max_column, colors, min_size_limit );
    });

    return;
}

function get_max_column( commit_rows ) {

    var max_column = 0;

    $(commit_rows).each(function () {
        if ( !this.columns ) return;
        if ( this.columns.length > max_column ) max_column = this.columns.length;
        return;
    });

    return max_column;
}

function get_min_size_limit ( commit_rows, max_column ) {

    var min_size_limit = 9999;

    $(commit_rows).each(function () {
        var canvas = this.canvas;
        if ( !canvas ) return;

        if ( canvas.height                 < min_size_limit ) min_size_limit = canvas.height;
        if ( canvas.width / max_column + 1 < min_size_limit ) min_size_limit = canvas.width / max_column + 1;

        return;
    });

    return min_size_limit;
}

function render_commit_row ( commit_row, commit_rows, max_column, colors, min_size_limit ) {

    var canvas = commit_row.canvas;
    if ( !canvas ) return;

    var commit_column_index = commit_row.commit_column_index;

    $($(commit_row.columns).get().reverse()).each(function () {
        var column = this;
        if ( !column ) return;
        if ( !column.visuals ) return;

        var cell = get_graph_cell( canvas, column.index, max_column, colors, min_size_limit );

        if ( column.visuals.commit ) {
            draw_commit( canvas, cell );

            var next_row = commit_rows[ commit_row.index + 1 ];
            if ( next_row ) {
                var next_column = next_row.columns[column.index];
                if ( next_column && ( next_column.visuals.commit || next_column.visuals.expects_commit || next_column.visuals.branch_point ) ) {
                    draw_down_connect( canvas, cell );
                }
            }

            if ( commit_row.index > 0 ) {
                var prev_row = commit_rows[ commit_row.index - 1 ];
                var prev_column = prev_row.columns[column.index];
                if ( prev_column && ( prev_column.visuals.commit || prev_column.visuals.expects_commit || prev_column.visuals.merge_point ) ) {
                    draw_up_connect( canvas, cell );
                }
            }
        }
        if ( column.visuals.expects_commit )      draw_expects( canvas, cell );
        if ( column.visuals.merge_point    )  draw_merge_point( canvas, cell );
        if ( column.visuals.branch_point   ) draw_branch_point( canvas, cell );

        if ( column.visuals.merge_point    ) draw_branch_merge_line( canvas, cell, column.index - commit_column_index - 1 );
        if ( column.visuals.branch_point   ) draw_branch_merge_line( canvas, cell, column.index - commit_column_index - 1 );

        return;
    });

    return;
}

function draw_down_connect(r, cell) {

    var start_height = r.height - ( cell.min_size_limit / 2 * 0.3 );

    r.path( "M{0} {1}L{0} {2}", cell.offset, start_height, r.height ).attr( cell.color );

    return;
}

function draw_up_connect(r, cell) {

    var end_height = cell.min_size_limit / 2 * 0.3;

    r.path( "M{0} 0L{0} {1}", cell.offset, end_height ).attr( cell.color );

    return;
}

function draw_expects(r, cell) {

    r.path( "M{0} 0L{0} {1}", cell.offset, r.height ).attr( cell.color );

    return;
}

function draw_commit(r, cell) {

    r.circle( cell.offset, r.height / 2, cell.min_size_limit / 2 * 0.7 ).attr( cell.color );

    return;
}

function draw_merge_point(r, cell) {

    var center_offset = cell.min_size_limit / 3;
    r.path(
        "M{0} {1}L{2} {3}L{4} {5}L{6} {7}", //
        cell.offset + -1 * cell.width / 2, r.height / 2, //
        cell.offset + -1 * center_offset,  r.height / 2, //
        cell.offset,                       r.height / 2 + center_offset, //
        cell.offset,                       r.height //
    ).attr( cell.color );

    return;
}

function draw_branch_point(r, cell) {

    var center_offset = cell.min_size_limit / 3;
    r.path(
        "M{0} {1}L{2} {3}L{4} {5}L{6} {7}", //
        cell.offset,                       0, //
        cell.offset,                       r.height / 2 - center_offset, //
        cell.offset + -1 * center_offset,  r.height / 2, //
        cell.offset + -1 * cell.width / 2, r.height / 2 //
    ).attr( cell.color );

    return;
}

function draw_branch_merge_line(r, cell, line_length) {

    var offset = cell.width / -2 + cell.offset;
    r.path(
        "M{0} {1}L{2} {3}", //
        offset - cell.width * line_length - cell.min_size_limit / 2 * 0.3, r.height / 2, //
        offset,                                                            r.height / 2
    ).attr( cell.color );

    return;
}

function get_graph_cell( r, column, max_column, colors, min_size_limit ) {
    var width = r.width / (max_column + 1);
    var offset = width * column;
    var size_limit = width;
    if ( r.height < size_limit ) size_limit = r.height;
    return {
        size_limit: size_limit,
        width: width,
        offset: width / 2 + offset,
        column: column,
        color: { stroke: colors[column % 8] },
        min_size_limit: min_size_limit
    };
}

function fill_colors() {
    var colors = new Array();

    colors[0] = "#000000";
    var color_index = 7;
    while ( color_index-- ) {
        Raphael.getColor();
        colors[color_index + 1] = Raphael.getColor();
    }

    return colors;
}

function render_canvas(id) {
    var target = $('#' + id);
    if ( !target.length ) return;
    var x = target.offset().left;
    var y = target.offset().top;
    var width = target.innerWidth();
    var height = target.innerHeight();

    target.css('padding', '0');
    target.css('line-height', '0');

    return Raphael(id, width, height);
}
@;
}

1;
