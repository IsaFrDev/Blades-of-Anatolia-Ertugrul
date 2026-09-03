using UnityEngine;

namespace Ertugrul.NPC
{
    /// <summary>
    /// Har qanday "E bosib ishlatish mumkin" bo'lgan obyekt shu interfeysni bajaradi:
    /// NPC, eshik, quduq, sandiq, temirxona, kodeks obyekti...
    /// </summary>
    public interface IErtInteractable
    {
        /// <summary>Ekranda ko'rinadigan matn — "Turgut bilan gaplashish"</summary>
        string GetPrompt();

        /// <summary>Hozir ishlatish mumkinmi? (masalan, quest hali ochilmagan)</summary>
        bool CanInteract();

        /// <summary>[E] bosilganda chaqiriladi</summary>
        void Interact(GameObject interactor);

        /// <summary>Prompt qayerda ko'rsatiladi (odatda boshning tepasi)</summary>
        Transform GetTransform();
    }
}
