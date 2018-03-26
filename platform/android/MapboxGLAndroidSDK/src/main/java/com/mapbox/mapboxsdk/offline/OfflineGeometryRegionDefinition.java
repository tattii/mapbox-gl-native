package com.mapbox.mapboxsdk.offline;

import android.os.Parcel;
import android.os.Parcelable;

import com.mapbox.geojson.Feature;
import com.mapbox.geojson.Geometry;
import com.mapbox.geojson.GeometryCollection;
import com.mapbox.geojson.LineString;
import com.mapbox.geojson.MultiLineString;
import com.mapbox.geojson.MultiPoint;
import com.mapbox.geojson.MultiPolygon;
import com.mapbox.geojson.Point;
import com.mapbox.geojson.Polygon;
import com.mapbox.mapboxsdk.geometry.LatLngBounds;
import com.mapbox.turf.TurfMeasurement;

import java.util.ArrayList;
import java.util.List;

/**
 * An offline region defined by a style URL, geometry, zoom range, and
 * device pixel ratio.
 * <p>
 * Both minZoom and maxZoom must be ≥ 0, and maxZoom must be ≥ minZoom.
 * <p>
 * maxZoom may be ∞, in which case for each tile source, the region will include
 * tiles from minZoom up to the maximum zoom level provided by that source.
 * <p>
 * pixelRatio must be ≥ 0 and should typically be 1.0 or 2.0.
 */
public class OfflineGeometryRegionDefinition implements OfflineRegionDefinition, Parcelable {

  private String styleURL;
  private Geometry geometry;
  private double minZoom;
  private double maxZoom;
  private float pixelRatio;

  /**
   * Constructor to create an OfflineTilePyramidDefinition from parameters.
   *
   * @param styleURL   the style
   * @param geometry   the geometry
   * @param minZoom    min zoom
   * @param maxZoom    max zoom
   * @param pixelRatio pixel ratio of the device
   */
  public OfflineGeometryRegionDefinition(
    String styleURL, Geometry geometry, double minZoom, double maxZoom, float pixelRatio) {
    // Note: Also used in JNI
    this.styleURL = styleURL;
    this.geometry = geometry;
    this.minZoom = minZoom;
    this.maxZoom = maxZoom;
    this.pixelRatio = pixelRatio;
  }

  /**
   * Constructor to create an OfflineTilePyramidDefinition from a Parcel.
   *
   * @param parcel the parcel to create the OfflineTilePyramidDefinition from
   */
  public OfflineGeometryRegionDefinition(Parcel parcel) {
    this.styleURL = parcel.readString();
    // TODO: https://github.com/mapbox/mapbox-java/pull/770
    this.geometry = Feature.fromJson(parcel.readString()).geometry();
    this.minZoom = parcel.readDouble();
    this.maxZoom = parcel.readDouble();
    this.pixelRatio = parcel.readFloat();
  }

  /*
   * Getters
   */

  public String getStyleURL() {
    return styleURL;
  }

  public Geometry getGeometry() {
    return geometry;
  }

  private double[] bbox(Geometry geometry) {
    if (geometry instanceof Point) {
      return TurfMeasurement.bbox((Point) geometry);
    } else if (geometry instanceof MultiPoint) {
      return TurfMeasurement.bbox((MultiPoint) geometry);
    } else if (geometry instanceof LineString) {
      return TurfMeasurement.bbox((LineString) geometry);
    } else if (geometry instanceof MultiLineString) {
      return TurfMeasurement.bbox((MultiLineString) geometry);
    } else if (geometry instanceof Polygon) {
      return TurfMeasurement.bbox((Polygon) geometry);
    } else if (geometry instanceof MultiPolygon) {
      return TurfMeasurement.bbox((MultiPolygon) geometry);
    } else if (geometry instanceof GeometryCollection) {
      List<Point> points = new ArrayList<>();
      for (Geometry geo : ((GeometryCollection) geometry).geometries()) {
        double[] bbox = bbox(geo);
        points.add(Point.fromLngLat(bbox[0], bbox[1]));
        points.add(Point.fromLngLat(bbox[2], bbox[1]));
        points.add(Point.fromLngLat(bbox[2], bbox[3]));
        points.add(Point.fromLngLat(bbox[0], bbox[3]));
      }
      return TurfMeasurement.bbox(MultiPoint.fromLngLats(points));
    } else {
      throw new RuntimeException(("Unknown geometry class: " + geometry.getClass()));
    }
  }

  /**
   * Calculates the bounding box for the Geometry it contains
   * to retain backwards compatibility
   * @return the {@link LatLngBounds} or null
   */
  @Override
  public LatLngBounds getBounds() {
    if (geometry == null) {
      return null;
    }

    // TODO: https://github.com/mapbox/mapbox-java/pull/769
    double[] bbox = bbox(geometry);
    return LatLngBounds.from(bbox[3], bbox[2], bbox[1], bbox[0]);
  }

  public double getMinZoom() {
    return minZoom;
  }

  public double getMaxZoom() {
    return maxZoom;
  }

  public float getPixelRatio() {
    return pixelRatio;
  }

  /*
   * Parceable
   */

  @Override
  public int describeContents() {
    return 0;
  }

  @Override
  public void writeToParcel(Parcel dest, int flags) {
    dest.writeString(styleURL);
    // TODO: https://github.com/mapbox/mapbox-java/pull/770
    dest.writeString(Feature.fromGeometry(geometry).toJson());
    dest.writeDouble(minZoom);
    dest.writeDouble(maxZoom);
    dest.writeFloat(pixelRatio);
  }

  public static final Creator CREATOR = new Creator() {
    public OfflineGeometryRegionDefinition createFromParcel(Parcel in) {
      return new OfflineGeometryRegionDefinition(in);
    }

    public OfflineGeometryRegionDefinition[] newArray(int size) {
      return new OfflineGeometryRegionDefinition[size];
    }
  };
}
