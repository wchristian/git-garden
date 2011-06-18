use strict;
use warnings;

package Git::Garden::Commit;

use Moo;

has uid => ( is => 'rw', required => 1 );
has sort_index => ( is => 'rw', required => 1 );
has parents => ( is => 'rw' );
has labels => ( is => 'rw' );
has merge_depth => ( is => 'rw' );

1;
