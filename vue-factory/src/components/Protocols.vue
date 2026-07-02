<template>
    <div :style="{
            '--text': (msg?.payload?.theme?.text || '#FFFFFF'),
            '--rgb0': (msg?.payload?.theme?.rgb_hex || '#000000'),
            '--rgb1': (msg?.payload?.theme?.rgb_hex || '#000000') + '44',
            '--rgb2': (msg?.payload?.theme?.rgb_hex || '#000000') + '99',
            '--rgb3': (msg?.payload?.theme?.rgb_hex || '#000000') + '22'
        }">

        <div class="flex justify-center">
            <div class="w-1 rounded-l-sm rounded-br-sm bg-[var(--rgb1)]"></div>

            <div class="w-full">
                <div class="bg-[var(--rgb1)] px-4 py-6 rounded-r-sm">
                    <h2 class="text-3xl font-bold text-[var(--text)]">
                        Протоколи
                    </h2>
                </div>

                <ul class="m-8 flex flex-col gap-2 list-none">
                    <li
                        v-for="(protocol, index) in availableProtocols"
                        :key="index"
                        class="rounded-md border border-[var(--rgb1)] overflow-hidden transition-all duration-300"
                    >
                        <div
                            class="group px-3 py-2 hover:bg-[var(--rgb1)] transition-all duration-300"
                        >
                            <label
                                class="flex items-center justify-between gap-5 cursor-pointer"
                            >
                            <div class="flex items-center gap-2 min-w-0">

                                <div
                                    class="flex items-center justify-center w-11 h-11 p-1 mr-2 rounded-xl border border-[var(--rgb1)] bg-[var(--rgb1)] text-xl shrink-0"
                                >
                                    <svg v-if="protocol.type === 'AUTO'" viewBox="0 0 24 24" fill="none" xmlns="http://www.w3.org/2000/svg"><g id="SVGRepo_bgCarrier" stroke-width="0"></g><g id="SVGRepo_tracerCarrier" stroke-linecap="round" stroke-linejoin="round"></g><g id="SVGRepo_iconCarrier"> <path d="M4.19995 18V11C4.19995 9.34315 5.5431 8 7.19995 8H16.8C18.4568 8 19.8 9.34315 19.8 11V18C19.8 19.6569 18.4568 21 16.8 21H7.19995C5.5431 21 4.19995 19.6569 4.19995 18Z" stroke="#a78b8b" stroke-width="1.7"></path> <path d="M12 5L12 8" stroke="#a78b8b" stroke-width="1.7" stroke-linecap="round"></path> <path d="M20 12H21C22.1046 12 23 12.8954 23 14V15.4459C23 16.2637 22.5021 16.9992 21.7428 17.3029L20 18" stroke="#a78b8b" stroke-width="1.7"></path> <path d="M4 12H3C1.89543 12 1 12.8954 1 14V15.4459C1 16.2637 1.4979 16.9992 2.25722 17.3029L4 18" stroke="#a78b8b" stroke-width="1.7"></path> <path fill-rule="evenodd" clip-rule="evenodd" d="M15.9999 11.15C16.4693 11.15 16.8499 11.5306 16.8499 12V14C16.8499 14.4695 16.4693 14.85 15.9999 14.85C15.5305 14.85 15.1499 14.4695 15.1499 14V12C15.1499 11.5306 15.5305 11.15 15.9999 11.15Z" fill="#a78b8b"></path> <path fill-rule="evenodd" clip-rule="evenodd" d="M7.9999 11.15C8.46934 11.15 8.8499 11.5306 8.8499 12V14C8.8499 14.4695 8.46934 14.85 7.9999 14.85C7.53046 14.85 7.1499 14.4695 7.1499 14V12C7.1499 11.5306 7.53046 11.15 7.9999 11.15Z" fill="#a78b8b"></path> <circle cx="12" cy="3" r="2" fill="#871742"></circle> <path d="M10 17H14" stroke="#a78b8b" stroke-width="1.7" stroke-linecap="round"></path> <path d="M22 12V10" stroke="#a78b8b" stroke-width="1.7" stroke-linecap="round"></path> <path d="M2 12V10" stroke="#a78b8b" stroke-width="1.7" stroke-linecap="round"></path> </g></svg>
                                    <svg v-else viewBox="0 0 16 16" fill="none" xmlns="http://www.w3.org/2000/svg"><g id="SVGRepo_bgCarrier" stroke-width="0"></g><g id="SVGRepo_tracerCarrier" stroke-linecap="round" stroke-linejoin="round"></g><g id="SVGRepo_iconCarrier"> <path d="M8 3V0H6V3H8Z" fill="#a78b8b"></path> <path d="M9.87868 12H7V15H10.6109C13.5872 15 16 12.5872 16 9.61091C16 8.18164 15.4322 6.8109 14.4216 5.80025L13.3322 4.71088L8.8322 6.71088L6.56066 4.43934L4.43934 6.56066L9.87868 12Z" fill="#a78b8b"></path> <path d="M0 6H3V8H0V6Z" fill="#a78b8b"></path> <path d="M3.29289 4.70711L0.792893 2.20711L2.20711 0.792893L4.70711 3.29289L3.29289 4.70711Z" fill="#a78b8b"></path> </g></svg>
                                </div>

                                <div class="flex flex-col min-w-0">
                                    <span class="text-xl font-semibold text-[var(--text)] break-all">
                                        {{ protocol.id }}
                                    </span>

                                    <span
                                        class="text-xs tracking-[0.25em] opacity-70"
                                    >
                                        {{ protocol.type }}
                                    </span>
                                </div>
                            </div>

                            <div class="flex gap-3 items-center">
                                <div class="flex items-center gap-4">
                                    <input
                                        type="checkbox"
                                        class="sr-only peer"
                                        v-model="selectedProtocols"
                                        :value="protocol.id"
                                        @change="handleProtocolToggle(protocol.id)"
                                    />
                                    <div
                                        class="w-11 h-6 shrink-0 flex items-center bg-[var(--text)] rounded-md p-1 peer-checked:bg-[var(--rgb0)] transition-colors duration-300 border border-[var(--rgb1)]"
                                    >

                                        <div
                                            class="w-4 h-4 rounded-md shadow-md transform transition-all duration-300"
                                            :class="selectedProtocols.includes(protocol.id)
                                                ? 'translate-x-5 bg-[var(--text)]'
                                                : 'bg-[var(--rgb0)]'"
                                        >
                                        </div>

                                    </div>
                                </div>
                            </div>

                        </label>

                            <div
                                class="grid transition-all duration-300 ease-in-out group-hover:grid-rows-[1fr] grid-rows-[0fr]"
                            >
                                <div class="overflow-hidden">
                                    <div
                                        class="mt-3 rounded-md border border-[var(--rgb1)] bg-[var(--rgb3)] px-4 py-3"
                                    >
                                        <div class="text-xs tracking-[0.2em] opacity-60 mb-2">
                                            {{ protocol.type }}
                                        </div>

                                        <p class="text-sm leading-relaxed opacity-90">
                                            {{ protocol.desc || 'Опис відсутній.' }}
                                        </p>
                                    </div>
                                </div>
                            </div>
                        </div>
                    </li>
                </ul>

                <div class="bg-[var(--rgb1)] h-1px px-4 py-6 rounded-r-sm"></div>
            </div>
        </div>
    </div>
</template>

<script>
export default {
    props: ["msg"],

    data() {
        return {
            availableProtocols: [],
            selectedProtocols: [],
        }
    },

    watch: {
        msg: {
            immediate: true,
            handler(newMsg) {
                let pData = newMsg?.payload?.protocol;

                if (pData) {
                    if (pData.allAvailable) {
                        this.availableProtocols = pData.allAvailable;
                    }

                    if (pData.allSelected !== undefined) {
                        this.selectedProtocols = pData.allSelected;
                    }
                }
            }
        }
    },

    methods: {
        handleProtocolToggle(selectedProtocol) {
            const isActive = this.selectedProtocols.includes(selectedProtocol);

            this.send({
                payload: {
                    protocol: selectedProtocol,
                    state: isActive,
                }
            });
        }
    },
}
</script>