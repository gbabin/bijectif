pkgname=bijectif
pkgver=1.5.1
pkgrel=1
pkgdesc='Edit image titles in filenames'
arch=('x86_64')
url='https://github.com/gbabin/bijectif'
license=('GPL3')
depends=(qt6-base qt6-multimedia)
makedepends=(cmake make qt6-tools)
optdepends=('qt6-imageformats: additional image formats')
source=("${pkgname}-${pkgver}.tar.gz::https://github.com/gbabin/bijectif/archive/v${pkgver}.tar.gz")
sha256sums=('SKIP')

build() {
  cd "${pkgname}-${pkgver}"

  cmake -B build -G 'Unix Makefiles' \
        -DCMAKE_INSTALL_PREFIX=/usr \
        -DCMAKE_BUILD_TYPE=RelWithDebInfo
  cmake --build build
}

check() {
  cd "${pkgname}-${pkgver}"

  cd tests
  cmake -B build -G 'Unix Makefiles'
  cmake --build build
  ./build/tester
}

package() {
  cd "${pkgname}-${pkgver}"

  DESTDIR="$pkgdir" cmake --install build
  install -Dm644 LICENSE "$pkgdir"/usr/share/licenses/bijectif/COPYING
  install -Dm644 bijectif.desktop "$pkgdir"/usr/share/applications/bijectif.desktop
}
