pkgname=bijectif
pkgver=1.8
pkgrel=1
pkgdesc='Edit image titles in filenames'
arch=('x86_64')
url='https://github.com/gbabin/bijectif'
license=('GPL3')
depends=(qt6-base qt6-multimedia)
makedepends=(cmake libexif make qt6-tools)
optdepends=('qt6-imageformats: additional image formats')
source=("${pkgname}-${pkgver}.tar.gz::https://github.com/gbabin/bijectif/archive/v${pkgver}.tar.gz")
sha256sums=('SKIP')

build() {
  cmake -B build \
        -S "${pkgname}-${pkgver}" \
        -W no-dev \
        -D CMAKE_BUILD_TYPE=None \
        -D CMAKE_INSTALL_PREFIX=/usr
  cmake --build build
}

check() {
  ctest --test-dir build/tests \
        --output-on-failure \
        --no-tests=error
}

package() {
  DESTDIR="$pkgdir" cmake --install build
  rm -r "$pkgdir"/usr/include/QGeoView
  rmdir "$pkgdir"/usr/include

  cd "${pkgname}-${pkgver}"
  install -Dm644 LICENSE "$pkgdir"/usr/share/licenses/bijectif/COPYING
  install -Dm644 src/QGeoView/LICENSE "$pkgdir"/usr/share/licenses/qgeoview/COPYING
  install -Dm644 bijectif.desktop "$pkgdir"/usr/share/applications/bijectif.desktop
}
