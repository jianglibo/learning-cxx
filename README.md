## ROI

![roi](./docs/imgs/image.png)

```cxx
    __host__ __device__
        thrust::tuple<SubImgData<T>, SubImgData<T>>
        operator()(thrust::tuple<int, int> roiCoord) const
    {
        int x = thrust::get<0>(roiCoord);
        int y = thrust::get<1>(roiCoord);
        int t = thrust::max(-imgDataA.roi.y, y - maxDisplacement);
        int l = thrust::max(-imgDataA.roi.x, x - maxDisplacement);

        // x,y are coordinates of ROI!!!!!!!
        int b = thrust::min(y + patchSize + maxDisplacement, imgDataA.maxDownRoiCoord());
        int r = thrust::min(x + patchSize + maxDisplacement, imgDataA.maxRightRoiCoord());

        Rc tplRc{x, y, patchSize, patchSize};
        Rc beSearchedRc{l, t, b - t, r - l};

        SubImgData<T> tplSubImgData{imgDataA, tplRc};
        SubImgData<T> beSeachedSubImgData{imgDataB, beSearchedRc};

        if (x + patchSize > imgDataA.maxRightRoiCoord() || y + patchSize > imgDataA.maxDownRoiCoord())
        {
            tplSubImgData.valid = false;
        }
        if (beSearchedRc.rows < patchSize || beSearchedRc.cols < patchSize)
        {
            tplSubImgData.valid = false;
        }

        return {tplSubImgData, beSeachedSubImgData};
    }
```