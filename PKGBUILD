pkgname=bijectif
pkgver=2.1
pkgrel=1
pkgdesc='Edit image titles in filenames'
arch=('x86_64')
url='https://github.com/gbabin/bijectif'
license=('GPL3')
depends=(ffmpeg libexif libheif qt6-base)
makedepends=(cmake make pkg-config qt6-tools)
optdepends=('kimageformats: additional image formats'
            'qt6-imageformats: additional image formats')
source=("${pkgname}-${pkgver}.tar.gz::https://github.com/gbabin/bijectif/archive/v${pkgver}.tar.gz"
        "QGeoView.tar.gz::https://github.com/gbabin/QGeoView/archive/9a8df787f001c995a9f2a8c164bbe202b8b5ff66.tar.gz")
sha256sums=('SKIP'
            'SKIP')

prepare() {
  rmdir "${pkgname}-${pkgver}/src/QGeoView"
  ln -s "${srcdir}"/QGeoView-*/ "${pkgname}-${pkgver}/src/QGeoView"
}

build() {
  cmake -B build \
        -S "${pkgname}-${pkgver}" \
        -W no-author \
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
  install -Dm644 src/KDSignalThrottler/MIT.txt "$pkgdir"/usr/share/licenses/bijectif/MIT.txt
  install -Dm644 src/QGeoView/LICENSE "$pkgdir"/usr/share/licenses/qgeoview/COPYING
  install -Dm644 bijectif.desktop "$pkgdir"/usr/share/applications/bijectif.desktop
}
