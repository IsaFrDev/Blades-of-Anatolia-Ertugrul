using System;
using System.Collections.Generic;
using UnityEngine;

namespace Ertugrul.Core
{
    /// <summary>
    /// Dunyo holati — o'yinchining butun tarixi shu yerda.
    /// GDD dagi `UErtWorldStateSubsystem` ning Unity versiyasi.
    ///
    /// Bu yerda saqlanadi:
    ///   • Flag'lar   — "Titus.Spared", "Nail.Taken" kabi bool holatlar
    ///   • Scalar'lar — Iymon, Sabr, oba darajasi kabi sonlar
    ///   • Fraksiya obro'si — 6 fraksiya, -100..+100
    /// </summary>
    [Serializable]
    public class ErtWorldState
    {
        // ── Flag'lar ────────────────────────────────────────────────
        [SerializeField] private List<string> _flags = new();

        // ── Sonli holatlar ──────────────────────────────────────────
        [SerializeField] private List<string> _scalarKeys = new();
        [SerializeField] private List<float>  _scalarValues = new();

        // ── Fraksiya obro'si ────────────────────────────────────────
        [SerializeField] private float[] _reputation = new float[6];

        public event Action<string, bool> OnFlagChanged;
        public event Action<ErtFaction, float> OnReputationChanged;

        // ═══════════════════════════════════════════════════════════
        //  FLAG'LAR
        // ═══════════════════════════════════════════════════════════

        public bool HasFlag(string flag) => _flags.Contains(flag);

        public void SetFlag(string flag, bool value = true)
        {
            bool had = _flags.Contains(flag);
            if (value && !had) _flags.Add(flag);
            else if (!value && had) _flags.Remove(flag);
            else return;

            OnFlagChanged?.Invoke(flag, value);
            Debug.Log($"[WorldState] Flag: {flag} = {value}");
        }

        // ═══════════════════════════════════════════════════════════
        //  SCALAR'LAR (Iymon, Sabr, oba darajasi...)
        // ═══════════════════════════════════════════════════════════

        public float GetScalar(string key, float defaultValue = 0f)
        {
            int i = _scalarKeys.IndexOf(key);
            return i >= 0 ? _scalarValues[i] : defaultValue;
        }

        public void SetScalar(string key, float value)
        {
            int i = _scalarKeys.IndexOf(key);
            if (i >= 0) _scalarValues[i] = value;
            else { _scalarKeys.Add(key); _scalarValues.Add(value); }
        }

        public void AddScalar(string key, float delta)
            => SetScalar(key, GetScalar(key) + delta);

        // ═══════════════════════════════════════════════════════════
        //  FRAKSIYA OBRO'SI
        // ═══════════════════════════════════════════════════════════

        /// <summary>
        /// Fraksiyalar dushmanlik matritsasi (GDD 04_CORE_SYSTEMS.md).
        /// Biror fraksiyada obro' oshsa — uning dushmanida tushadi.
        /// Neytral qolib bo'lmaydi. Bu — o'yinning asosiy siyosiy qoidasi.
        /// </summary>
        private static readonly float[,] Enmity = {
            //        Selj  Ayyu  Temp  Mong  Ahi   Byz
            /*Selj*/ { 0f,  -.3f, -.4f, -.9f, +.5f, -.2f },
            /*Ayyu*/ {-.3f,  0f,  -.8f, -.7f, +.3f, -.1f },
            /*Temp*/ {-.4f, -.8f,  0f,  -.2f,  0f,  -.5f },
            /*Mong*/ {-.9f, -.7f, -.2f,  0f,  -.6f, -.4f },
            /*Ahi */ {+.5f, +.3f,  0f,  -.6f,  0f,  +.2f },
            /*Byz */ {-.2f, -.1f, -.5f, -.4f, +.2f,  0f  },
        };

        public float GetReputation(ErtFaction f) => _reputation[(int)f];

        public void AddReputation(ErtFaction faction, float delta)
        {
            int fi = (int)faction;
            _reputation[fi] = Mathf.Clamp(_reputation[fi] + delta, -100f, 100f);
            OnReputationChanged?.Invoke(faction, _reputation[fi]);

            // Teskari ta'sir — dushman fraksiyalarda
            for (int i = 0; i < 6; i++)
            {
                if (i == fi) continue;
                float e = Enmity[fi, i];
                if (Mathf.Approximately(e, 0f)) continue;

                _reputation[i] = Mathf.Clamp(_reputation[i] + delta * e * 0.5f, -100f, 100f);
                OnReputationChanged?.Invoke((ErtFaction)i, _reputation[i]);
            }
        }
    }

    public enum ErtFaction
    {
        Seljuk = 0,
        Ayyubid = 1,
        Templar = 2,
        Mongol = 3,
        Ahi = 4,
        Byzantine = 5
    }
}
