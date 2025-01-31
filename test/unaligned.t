Make sure we have a fresh build:

  $ export BUILDDIR=$TESTDIR/../build
  $ cd $BUILDDIR
  $ make 2> /dev/null >/dev/null
  $ cd $CRAMTMP
  $ export STAGEDIR=seek_stagedir
  $ export OUTPUT=seek_output

Create an input file, with size not a multiple of block-size

  $ mkdir $OUTPUT $STAGEDIR
  $ seq 123 > $STAGEDIR/file
  $ shasum $STAGEDIR/file
  09661402322f10d8954dc5a23e7dc615127f4418  */file (glob)
  $ dd status=noxfer if=/dev/zero bs=1023 count=1000 of=$STAGEDIR/in.hfs
  1000+0 records in
  1000+0 records out
  $ wc -c $STAGEDIR/in.hfs
  1023000 */in.hfs (glob)
  $ mkfs.hfsplus $STAGEDIR/in.hfs
  Initialized seek_stagedir/in.hfs as a 999 KB HFS Plus volume
  $ $BUILDDIR/hfs/hfsplus $STAGEDIR/in.hfs add $STAGEDIR/file file
  $ ORIG_SHA=$(shasum $STAGEDIR/in.hfs | awk '{print $1}')

Create a block-padded version for comparison

  $ dd status=noxfer if=$STAGEDIR/in.hfs bs=512 conv=sync of=$STAGEDIR/padded.hfs
  1998+1 records in
  1999+0 records out
  $ wc -c $STAGEDIR/padded.hfs
  1023488 */padded.hfs (glob)
  $ PADDED_SHA=$(shasum $STAGEDIR/padded.hfs | awk '{print $1}')
  $ test "$PADDED_SHA" != "$ORIG_SHA"

Convert to dmg

  $ $BUILDDIR/dmg/dmg dmg -c zlib $STAGEDIR/in.hfs $OUTPUT/out.dmg < $STAGEDIR/in.hfs > /dev/null

Test we can round-trip back to raw. Should be same size/contents as padded

  $ $BUILDDIR/dmg/dmg iso $OUTPUT/out.dmg $OUTPUT/out.hfs >/dev/null
  $ wc -c $OUTPUT/out.hfs
  1023488 */out.hfs (glob)
  $ OUT_SHA=$(shasum $OUTPUT/out.hfs | awk '{print $1}')
  $ test "$OUT_SHA" = "$PADDED_SHA"
  $ $BUILDDIR/hfs/hfsplus $OUTPUT/out.hfs cat file | shasum
  09661402322f10d8954dc5a23e7dc615127f4418  -
