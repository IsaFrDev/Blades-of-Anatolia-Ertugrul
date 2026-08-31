package com.fayzinc.ertugrul.common;

import jakarta.persistence.Column;
import jakarta.persistence.EntityListeners;
import jakarta.persistence.MappedSuperclass;
import org.springframework.data.annotation.CreatedDate;
import org.springframework.data.annotation.LastModifiedDate;
import org.springframework.data.jpa.domain.support.AuditingEntityListener;

import java.time.Instant;

/**
 * Base class for entities that carry creation/modification timestamps.
 *
 * <p>Auditing Spring Data JPA orqali ({@code @EnableJpaAuditing}) to'ldiriladi,
 * DB {@code DEFAULT now()} esa migratsiyalarda saqlanib qoladi — chunki ba'zi
 * yozuvlar (masalan telemetriya rollup'lari) JPA'ni chetlab o'tib, to'g'ridan
 * to'g'ri native UPSERT bilan yoziladi. Ikki qatlamda ham default bo'lishi —
 * ataylab qilingan: qaysi yo'l bilan yozilmasin, vaqt belgisi bor.
 *
 * <p>Deliberately does <b>not</b> define an {@code id}: entities here use
 * different id strategies (UUID for player-owned rows, composite natural keys
 * for rollups), so a shared id would be wrong more often than right.
 */
@MappedSuperclass
@EntityListeners(AuditingEntityListener.class)
public abstract class Auditable {

    @CreatedDate
    @Column(name = "created_at", nullable = false, updatable = false)
    private Instant createdAt;

    @LastModifiedDate
    @Column(name = "updated_at", nullable = false)
    private Instant updatedAt;

    public Instant getCreatedAt() {
        return createdAt;
    }

    public Instant getUpdatedAt() {
        return updatedAt;
    }

    /**
     * Only for tests and data-migration jobs that must reproduce historical
     * timestamps. Production code must let auditing do its work.
     */
    protected void setCreatedAt(Instant createdAt) {
        this.createdAt = createdAt;
    }

    protected void setUpdatedAt(Instant updatedAt) {
        this.updatedAt = updatedAt;
    }
}
