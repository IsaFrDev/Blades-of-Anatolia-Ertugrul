// Animatsiya hodisalari qabul qiluvchisi: OnFootstep / OnLand (Starter Assets va Mixamo
// kliplarida bor). Qadam va qo'nish tovushlari protsedural (WAV kerak emas).
using UnityEngine;

namespace Ertugrul
{
    public class ErtugrulFootsteps : MonoBehaviour
    {
        public float volume = 0.35f;
        AudioSource src;
        AudioClip step, land;

        void Awake()
        {
            src = gameObject.AddComponent<AudioSource>();
            src.spatialBlend = 0.6f; src.playOnAwake = false;
            step = Make("step", 0.09f, 900f, 0.010f, 1);
            land = Make("land", 0.22f, 220f, 0.040f, 3);
        }

        // Qisqa shovqin + past ton — yumshoq tuproq qadami
        static AudioClip Make(string name, float sec, float hz, float tau, int seed)
        {
            int sr = 22050, n = (int)(sr * sec);
            var d = new float[n];
            var rnd = new System.Random(seed);
            float lp = 0f;
            for (int i = 0; i < n; ++i)
            {
                float t = i / (float)sr;
                float env = Mathf.Exp(-t / tau) ;
                float noise = (float)(rnd.NextDouble() * 2 - 1);
                lp += (noise - lp) * 0.18f;                      // past chastotali shovqin
                d[i] = (lp * 0.8f + Mathf.Sin(t * hz * 6.2832f) * 0.35f * Mathf.Exp(-t / (tau * 2f))) * env;
            }
            var c = AudioClip.Create(name, n, 1, sr, false);
            c.SetData(d, 0);
            return c;
        }

        // AnimationEvent nomlari kliplardagi bilan bir xil bo'lishi shart
        public void OnFootstep(AnimationEvent e)
        {
            if (e.animatorClipInfo.weight < 0.5f) return;      // aralashishda ikki marta chalinmasin
            src.pitch = Random.Range(0.9f, 1.1f);
            src.PlayOneShot(step, volume);
        }
        public void OnLand(AnimationEvent e)
        {
            if (e.animatorClipInfo.weight < 0.5f) return;
            src.pitch = 1f;
            src.PlayOneShot(land, volume * 1.4f);
        }
    }
}
