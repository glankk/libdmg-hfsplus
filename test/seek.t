Make sure we have a fresh build:

  $ export BUILDDIR=$TESTDIR/../build
  $ cd $BUILDDIR
  $ make 2> /dev/null >/dev/null
  $ cd $CRAMTMP
  $ export STAGEDIR=seek_stagedir
  $ export OUTPUT=seek_output

Create an input file of reasonable size

  $ mkdir $OUTPUT $STAGEDIR
  $ seq 12345678 > $STAGEDIR/file
  $ shasum $STAGEDIR/file
  2ddc2f6d197392f11e5d1c6bfee0a80d95fdc36d  */file (glob)
  $ dd status=noxfer if=/dev/zero bs=1M count=120 of=$STAGEDIR/in.hfs
  120+0 records in
  120+0 records out
  $ mkfs.hfsplus $STAGEDIR/in.hfs
  Initialized seek_stagedir/in.hfs as a 120 MB HFS Plus volume
  $ $BUILDDIR/hfs/hfsplus $STAGEDIR/in.hfs add $STAGEDIR/file file
  $ SHASUM=$(shasum $STAGEDIR/in.hfs | awk '{print $1}')

Convert to dmg with seeking

  $ $BUILDDIR/dmg/dmg dmg -c zlib - $OUTPUT/out.dmg < $STAGEDIR/in.hfs > /dev/null

Test we can round-trip back to raw

  $ $BUILDDIR/dmg/dmg iso $OUTPUT/out.dmg $OUTPUT/out.hfs >/dev/null
  $ test "$(shasum $OUTPUT/out.hfs | awk '{print $1}')" = "$SHASUM"
  $ $BUILDDIR/hfs/hfsplus $OUTPUT/out.hfs cat file | shasum
  2ddc2f6d197392f11e5d1c6bfee0a80d95fdc36d  -
