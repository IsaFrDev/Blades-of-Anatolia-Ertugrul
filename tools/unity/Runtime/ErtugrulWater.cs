// Suv yuzasi: normal-xarita sekin oqadi — yaltirash jonlanadi.
using UnityEngine;

namespace Ertugrul
{
    public class ErtugrulWater : MonoBehaviour
    {
        public Vector2 speed = new Vector2(0.012f, 0.007f);
        MaterialPropertyBlock mpb;
        Renderer r;
        void Awake() { r = GetComponent<Renderer>(); mpb = new MaterialPropertyBlock(); }
        void Update()
        {
            if (r == null) return;
            r.GetPropertyBlock(mpb);
            float t = Time.time;
            mpb.SetVector("_BumpMap_ST", new Vector4(8f, 8f, t * speed.x, t * speed.y));
            r.SetPropertyBlock(mpb);
        }
    }
}
