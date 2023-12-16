pkgname=bijectif
pkgver=1.2
pkgrel=1
pkgdesc='Edit image titles in filenames'
arch=('x86_64')
url='https://github.com/gbabin/bijectif'
license=('GPL3')
depends=(qt6-base)
makedepends=(cmake git make qt6-tools)
optdepends=('qt6-imageformats: additional image formats')
source=("git+https://github.com/gbabin/bijectif.git#tag=v${pkgver}")
sha256sums=('SKIP')

build() {
  cd "$srcdir/bijectif"

  cmake -B build -G 'Unix Makefiles' \
        -DCMAKE_INSTALL_PREFIX=/usr \
        -DCMAKE_BUILD_TYPE=RelWithDebInfo
  cmake --build build
}

check() {
  cd "$srcdir/bijectif"

  cd tests
  cmake -B build -G 'Unix Makefiles'
  cmake --build build
  ./build/tester
}

package() {
  cd "$srcdir/bijectif"

  DESTDIR="$pkgdir" cmake --install build
  install -Dm644 LICENSE "$pkgdir"/usr/share/licenses/bijectif/COPYING
  install -Dm644 bijectif.desktop "$pkgdir"/usr/share/applications/bijectif.desktop
}
