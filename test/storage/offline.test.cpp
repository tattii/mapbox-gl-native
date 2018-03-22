#include <mbgl/storage/offline.hpp>
#include <mbgl/tile/tile_id.hpp>

#include <gtest/gtest.h>

using namespace mbgl;
using SourceType = mbgl::style::SourceType;

static const LatLngBounds sanFrancisco =
    LatLngBounds::hull({ 37.6609, -122.5744 }, { 37.8271, -122.3204 });

static const LatLngBounds sanFranciscoWrapped =
    LatLngBounds::hull({ 37.6609, 238.5744 }, { 37.8271, 238.3204 });

TEST(OfflineTilePyramidRegionDefinition, TileCoverEmpty) {
    OfflineTilePyramidRegionDefinition region("", LatLngBounds::empty(), 0, 20, 1.0);

    EXPECT_EQ((std::vector<CanonicalTileID>{}), region.tileCover(SourceType::Vector, 512, { 0, 22 }));
}

TEST(OfflineTilePyramidRegionDefinition, TileCoverZoomIntersection) {
    OfflineTilePyramidRegionDefinition region("", sanFrancisco, 2, 2, 1.0);

    EXPECT_EQ((std::vector<CanonicalTileID>{ { 2, 0, 1 } }),
              region.tileCover(SourceType::Vector, 512, { 0, 22 }));

    EXPECT_EQ((std::vector<CanonicalTileID>{}), region.tileCover(SourceType::Vector, 512, { 3, 22 }));
}

TEST(OfflineTilePyramidRegionDefinition, TileCoverTileSize) {
    OfflineTilePyramidRegionDefinition region("", LatLngBounds::world(), 0, 0, 1.0);

    EXPECT_EQ((std::vector<CanonicalTileID>{ { 0, 0, 0 } }),
              region.tileCover(SourceType::Vector, 512, { 0, 22 }));

    EXPECT_EQ((std::vector<CanonicalTileID>{ { 1, 0, 0 }, { 1, 0, 1 }, { 1, 1, 0 }, { 1, 1, 1 } }),
              region.tileCover(SourceType::Vector, 256, { 0, 22 }));
}

TEST(OfflineTilePyramidRegionDefinition, TileCoverZoomRounding) {
    OfflineTilePyramidRegionDefinition region("", sanFrancisco, 0.6, 0.7, 1.0);

    EXPECT_EQ((std::vector<CanonicalTileID>{ { 0, 0, 0 } }),
              region.tileCover(SourceType::Vector, 512, { 0, 22 }));

    EXPECT_EQ((std::vector<CanonicalTileID>{ { 1, 0, 0 } }),
              region.tileCover(SourceType::Raster, 512, { 0, 22 }));
}

TEST(OfflineTilePyramidRegionDefinition, TileCoverWrapped) {
    OfflineTilePyramidRegionDefinition region("", sanFranciscoWrapped, 0, 0, 1.0);

    EXPECT_EQ((std::vector<CanonicalTileID>{ { 0, 0, 0 } }),
              region.tileCover(SourceType::Vector, 512, { 0, 22 }));
}

TEST(OfflineTilePyramidRegionDefinition, TileCount) {
    OfflineTilePyramidRegionDefinition region("", sanFranciscoWrapped, 0, 22, 1.0);

    //These numbers match the count from tileCover().size().
    EXPECT_EQ(38424u, region.tileCount(SourceType::Vector, 512, { 10, 18 }));
    EXPECT_EQ(9675240u, region.tileCount(SourceType::Vector, 512, { 3, 22 }));
}

TEST(OfflineRegionDefinition, Point) {
    OfflineGeometryRegionDefinition region("", Point<double>(-122.5744, 37.6609), 0, 2, 1.0);
    
    EXPECT_EQ((std::vector<CanonicalTileID>{ { 0, 0, 0 }, { 1, 0, 0 }, {2 ,0 ,1 } }),
              region.tileCover(SourceType::Vector, 512, { 0, 22 }));
}

TEST(OfflineRegionDefinition, MultiPoint) {
    OfflineGeometryRegionDefinition region("", MultiPoint<double>{ { -122.5, 37.76 }, { -122.4, 37.76} }, 19, 20, 1.0);

    EXPECT_EQ((std::vector<CanonicalTileID>{
        {19, 83740, 202675}, {19, 83886, 202675},
        {20, 167480, 405351}, {20, 167772, 405351} }),
              region.tileCover(SourceType::Vector, 512, { 0, 22 }));
}

TEST(OfflineRegionDefinition, LineString) {
    OfflineGeometryRegionDefinition region("", LineString<double>{{ -122.5, 37.76 }, { -122.4, 37.76} }, 11, 14, 1.0);
    
    EXPECT_EQ((std::vector<CanonicalTileID>{
        {11, 327, 791},
        {12, 654, 1583}, {12, 655, 1583},
        {13, 1308, 3166}, {13, 1309, 3166}, {13, 1310, 3166},
        {14, 2616, 6333}, {14, 2617, 6333}, {14, 2618, 6333}, {14, 2619, 6333}, {14, 2620, 6333}, {14, 2621, 6333} }),
              region.tileCover(SourceType::Vector, 512, { 0, 22 }));
}

TEST(OfflineRegionDefinition, MultiLineString) {
    OfflineGeometryRegionDefinition region("", MultiLineString<double>{
        { { -122.5, 37.76 }, { -122.4, 37.76} },
        { { -122.5, 37.72 }, { -122.4, 37.72} } },
                                   13, 14, 1.0);

    EXPECT_EQ((std::vector<CanonicalTileID>{
              {13, 1308, 3166}, {13, 1309, 3166}, {13, 1310, 3166}, {13, 1308, 3167}, {13, 1309, 3167}, {13, 1310, 3167},
              {14, 2616, 6333}, {14, 2617, 6333}, {14, 2618, 6333}, {14, 2619, 6333}, {14, 2620, 6333},
              {14, 2621, 6333}, {14, 2616, 6335}, {14, 2617, 6335}, {14, 2618, 6335}, {14, 2619, 6335}, {14, 2620, 6335}, {14, 2621, 6335} }),
              region.tileCover(SourceType::Vector, 512, { 0, 22 }));
}

TEST(OfflineRegionDefinition, Polygon) {
    OfflineGeometryRegionDefinition region(
            "",
            Polygon<double>{ {
                 { -122.54219055175781, 37.658819317731265 },
                 { -122.3382568359375, 37.658819317731265 },
                 { -122.3382568359375, 37.810326435534755 },
                 { -122.54219055175781, 37.810326435534755 },
                 { -122.54219055175781, 37.658819317731265 }
             }}, 12, 12, 1.0);

    EXPECT_EQ((std::vector<CanonicalTileID>{
              {12, 653, 1582}, {12, 654, 1582}, {12, 655, 1582}, {12, 656, 1582}, {12, 653, 1583}, {12, 654, 1583},
              {12, 655, 1583}, {12, 656, 1583}, {12, 653, 1584}, {12, 654, 1584}, {12, 655, 1584}, {12, 656, 1584} }),
              region.tileCover(SourceType::Vector, 512, { 0, 22 }));
}


TEST(OfflineRegionDefinition, MultiPolygon) {
    OfflineGeometryRegionDefinition region(
            "",
            MultiPolygon<double>{{{
                      {-122.54219055175781, 37.7},
                      {-122.3382568359375, 37.7},
                      {-122.3382568359375, 37.8},
                      {-122.54219055175781, 37.8},
                      {-122.54219055175781, 37.7}
              }}, {{
                      {-122.54219055175781, 37.6},
                      {-122.3382568359375, 37.6},
                      {-122.3382568359375, 37.7},
                      {-122.54219055175781, 37.7},
                      {-122.54219055175781, 37.6}
              }}}, 12, 12, 1.0);
    EXPECT_EQ((std::vector<CanonicalTileID>{
              {12, 653, 1582}, {12, 654, 1582}, {12, 655, 1582}, {12, 656, 1582}, {12, 653, 1583}, {12, 654, 1583}, {12, 655, 1583},
              {12, 656, 1583}, {12, 653, 1584}, {12, 654, 1584}, {12, 655, 1584}, {12, 656, 1584}, {12, 653, 1585}, {12, 654, 1585},
              {12, 655, 1585}, {12, 656, 1585} }),
              region.tileCover(SourceType::Vector, 512, { 0, 22 }));
}

